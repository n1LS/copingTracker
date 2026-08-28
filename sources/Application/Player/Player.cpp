/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "Player.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Model/Groove.h"
#include "Application/Player/TablePlayback.h"
#include "Application/Utils/char.h"
#include "Application/Views/BaseClasses/ViewEvent.h"
#include "PlayerMixer.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/n_assert.h"
#include "System/System/System.h"
#include "System/io/Status.h"
#include <math.h>
#include <string.h>

int32_t DecodeRetriggerOffset(int32_t value) {
  return static_cast<int8_t>(value & 0xFF);
}

// Private constructor - Singleton

Player::Player() : mixer_() {
  isRunning_ = false;
  viewData_ = 0;

  lastSongPos_ = 0;
  mode_ = PM_SONG;
  sequencerMode_ = SM_SONG;
  lastPercentage_ = 0;
  retrigAllImmediate_ = false;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    instrumentOnChannel_[i] = NO_INSTRUMENT;
  }
}

bool Player::Init(Project *project, ViewData *viewData) {
  viewData_ = viewData;
  project_ = project;

  if (!mixer_.Init(project)) {
    return false;
  }

  mixer_.AddObserver((*this));
  SyncMaster *sync = SyncMaster::GetInstance();
  sync->SetTempo(project_->GetTempo());
  return mixer_.Start();
}

void Player::BindProject(Project *project, ViewData *viewData) {
  viewData_ = viewData;
  project_ = project;
  mixer_.BindProject(project);

  SyncMaster *sync = SyncMaster::GetInstance();
  sync->SetTempo(project_->GetTempo());
}

void Player::Reset() {
  viewData_ = 0;
  project_ = 0;
  mixer_.RemoveObserver(*this);
  Close();
}

void Player::Close() {
  mixer_.Stop();
  mixer_.Close();
}

void Player::SetChannelMute(int channel, bool mute) {
  mixer_.SetChannelMute(channel, mute);
}

bool Player::IsChannelMuted(int channel) {
  return mixer_.IsChannelMuted(channel);
}

void Player::Start(PlayMode mode, bool forceSongMode, MixerServiceMode msmMode, bool stopAtEnd) {

  mixer_.Lock();

  lastBeatCount_ = 0;
  stopAtEnd_ = stopAtEnd;

  // Get start time for clock

  System *system = System::GetInstance();
  now_ = startClock_ = system->GetClock();

  // Sets play mode.
  // DO I need playMode_ in view data ?
  // Seems like duplicate with mode_

  viewData_->playMode_ = (forceSongMode ? PM_SONG : mode);

  // See if we start from current song position
  // or from last stored

  unsigned playPos = viewData_->songY_ + viewData_->songOffset_;

  if (forceSongMode == false) {
    lastSongPos_ = playPos;
  } else {
    playPos = lastSongPos_;
  }

  // Clear all channel based data

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    mixer_.StopChannel(i);
    timeToLive_[i] = 0;
    timeToStart_[i] = 0;
    TablePlayback &tpb = TablePlayback::GetTablePlayback(i);
    TablePlayback &atp = TablePlayback::GetAutomationPlayback(i);
    tpb.Stop();
    atp.Stop();
  }

  // Tell the instruments we're starting

  project_->GetInstrumentBank()->OnStart();

  Groove::GetInstance()->Reset();

  // Let's get started !

  SyncMaster::GetInstance()->Start();
  SetAudioActive(true);

  firstPlayCycle_ = true;
  mode_ = viewData_->playMode_;

  // Notify MixerService about player start - this will set up rendering if
  // needed
  mixer_.OnPlayerStart(msmMode);

  MidiService *ms = MidiService::GetInstance();
  ms->OnPlayerStart();

  switch (viewData_->playMode_) {
    case PM_SONG:
      {
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          mixer_.StartChannel(i);
          updateSongPos(playPos, i);
        }
      }
      break;

    case PM_LIVE:
      {
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          if ((liveQueueingMode_[i] == QM_CHAINSTART) || (liveQueueingMode_[i] == QM_PHRASESTART) ||
              (liveQueueingMode_[i] == QM_TICKSTART)) {
            mixer_.StartChannel(i);
            updateSongPos(liveQueuePosition_[i], i, liveQueueChainPosition_[i]);
            liveQueueingMode_[i] = QM_NONE;
          }
        }
        break;
      }

    case PM_CHAIN:
    case PM_PHRASE:
      {
        int currentChannel = viewData_->songX_;
        mixer_.StartChannel(currentChannel);
        ;
        int currentChainPos = viewData_->chainRow_;
        updateSongPos(playPos, currentChannel, currentChainPos);
        break;
      }

    case PM_AUDITION:
      {
        int currentChannel = viewData_->songX_;
        mixer_.StartChannel(currentChannel);

        int currentChainPos = viewData_->chainRow_;
        int currentPhrasePos = viewData_->phraseCurPos_;
        // uses hop for PhrasePos
        updateSongPos(playPos, currentChannel, currentChainPos, currentPhrasePos);
        break;
      }

    default:
      NInvalid;
      break;
  }

  Trace::Error("USELESS?");

  startTime_ = mixer_.GetAudioOut()->GetStreamTime();

  SetChanged();
  PlayerEvent pe(PET_START);
  NotifyObservers(&pe);

  isRunning_ = true; // keep last !!!!

  mixer_.Unlock();
}

void Player::Stop() {
  mixer_.Lock();

  bool keepAudioActive = mixer_.IsPlaying();

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    mixer_.StopChannel(i);
    TablePlayback::GetTablePlayback(i).Stop();
    TablePlayback::GetAutomationPlayback(i).Stop();
  }
  MidiService::GetInstance()->OnPlayerStop();
  mixer_.OnPlayerStop();

  SyncMaster::GetInstance()->Stop();
  isRunning_ = false;
  SetChanged();
  PlayerEvent pe(PET_STOP);
  NotifyObservers(&pe);
  if (!keepAudioActive) {
    SetAudioActive(false);
  }

  mixer_.Unlock();
}

int Player::GetChannelNote(int channel) {
  return mixer_.GetChannelNote(channel);
}

uint8_t Player::GetPlayedInstrument(int channel) {
  if (!IsChannelMuted(channel)) {
    return instrumentOnChannel_[channel];
  }

  return NO_INSTRUMENT;
}

bool Player::GetPlayedSliceIndex(int channel, uint8_t &sliceIndex) {
  return mixer_.GetPlayedSliceIndex(channel, sliceIndex);
}

const char *Player::GetLiveIndicator(int channel) {

  bool blink = true;

  switch (liveQueueingMode_[channel]) {
    case QM_CHAINSTART:
    case QM_CHAINSTOP:
      blink = (now_ - startClock_) % 500 < 250;
      break;
    case QM_PHRASESTART:
    case QM_PHRASESTOP:
      blink = (now_ - startClock_) % 125 < 72;
      break;
    case QM_TICKSTART:
      blink = (now_ - startClock_) % 75 < 37;
      break;
    case QM_NONE:
      break;
  };
  if (blink) {
    switch (liveQueueingMode_[channel]) {
      case QM_CHAINSTART:
      case QM_PHRASESTART:
      case QM_TICKSTART:
        return IsChannelMuted(channel) ? char_indicator_positionMuted_s : char_indicator_position_s;
      case QM_CHAINSTOP:
      case QM_PHRASESTOP:
        return char_playback_pause_s;
        break;
      case QM_NONE:
        break;
    }
  }
  return " ";
}

void Player::SetSequencerMode(SequencerMode mode) {
  if (isRunning_) {
    switch (mode) {
      case SM_LIVE:
        mode_ = PM_LIVE;
        break;
      case SM_SONG:
        mode_ = PM_SONG;
        break;
    };
  };
  sequencerMode_ = mode;
}

SequencerMode Player::GetSequencerMode() {
  return sequencerMode_;
}

bool Player::IsChannelPlaying(int channel) {
  return mixer_.IsChannelPlaying(channel);
}

// Handles start button on any screen BUT the song screen

void Player::OnStartButton(PlayMode origin, unsigned int from, bool startFromPrevious, unsigned char chainPos,
                           MixerServiceMode msmMode, bool stopAtEnd) {

  switch (GetSequencerMode()) {

    case SM_SONG:

      // If sequencer not running, start otherwise stop

      if (isRunning_ && viewData_->playMode_ != PM_AUDITION) {
        Stop();
      } else {
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          liveQueueingMode_[i] = QM_NONE;
        };
        Start(origin, startFromPrevious, msmMode);
      }
      break;
    case SM_LIVE: // doesn't make much sense here
      break;
  }
}

// Handles start on song screen
void Player::OnSongStartButton(unsigned int from, unsigned int to, bool requestStop, bool forceImmediate,
                               MixerServiceMode msmMode, bool stopAtEnd) {

  switch (GetSequencerMode()) {

    case SM_SONG:

      // If sequencer not running, start otherwise stop

      if (isRunning_ && viewData_->playMode_ != PM_AUDITION) {
        if (!forceImmediate) {
          Stop();
        } else {
          // Get current song row and queue for immediate retrigger
          retrigPos_ = viewData_->songY_ + viewData_->songOffset_;
          retrigAllImmediate_ = true;
        }
      } else {
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          liveQueueingMode_[i] = QM_NONE;
        };
        Start(PM_SONG, false, msmMode);
      }
      break;

    case SM_LIVE:

      // Get current song row
      unsigned char songPos = viewData_->songY_ + viewData_->songOffset_;

      if (!IsRunning()) {

        // not playing; we queue the chains in the selection
        // that contain something then start the player

        for (unsigned int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          if ((i < from) || (i > to)) {
            QueueChannel(i, QM_NONE, 0);
          } else {
            if (isPlayable(songPos, i, 0)) {
              QueueChannel(i, QM_CHAINSTART, songPos, 0);
            }
          }
        };

        Start(PM_LIVE, false, msmMode);

      } else { // Player already running

        // Queue all chain in the given selection

        for (unsigned int i = from; i < to + 1; i++) {

          QueueingMode mode = QM_NONE;

          uint8_t row = songPos;

          if (!requestStop) {
            if (findPlayable(&row, i, 0)) {
              if (!forceImmediate) {
                if ((liveQueueingMode_[i] != QM_CHAINSTART) || (liveQueuePosition_[i] != row)) {
                  mode = QM_CHAINSTART;
                } else {
                  mode = QM_PHRASESTART;
                }
              } else {
                mode = QM_TICKSTART;
              }
            }
          } else { // modifier = onStop from song screen
            if (GetQueueingMode(i) != QM_CHAINSTOP) {
              mode = QM_CHAINSTOP;
            } else {
              mode = QM_PHRASESTOP;
            }
          }
          if (mode != QM_NONE) {
            QueueChannel(i, mode, row, 0);
          }
        }
      };
      break;
  }
}

bool Player::IsRunning() {
  return isRunning_;
}

stereosample Player::GetMasterLevel() {
  return mixer_.GetMasterOutLevel();
}

bool Player::isPlayable(int row, int col, int chainPos) {

  uint8_t *chain = &viewData_->song_->rows_[row].chains[col];
  if (*chain != 0xFF) {
    uint8_t data = viewData_->song_->chain_.steps_[*chain][chainPos].phrase;
    return (data != 0xFF);
  }
  return false;
}

bool Player::findPlayable(uint8_t *row, int col, uint8_t chainPos) {

  // first look if current is fine

  uint8_t *chain = &viewData_->song_->rows_[*row].chains[col];
  if (*chain != 0xFF) {
    uint8_t data = viewData_->song_->chain_.steps_[*chain][chainPos].phrase;
    return (data != 0xFF);
  }

  while (*row != 255) {
    chain = &viewData_->song_->rows_[*row].chains[col];
    if (*chain != 0xFF) {
      break;
    }
    *row -= 1;
  }

  if (*row == 255)
    return false;

  // Find upwards the first blank

  while (*row != 255) {
    chain = &viewData_->song_->rows_[*row].chains[col];
    if (*chain == 0xFF) {
      break;
    }
    *row -= 1;
  }

  if (*row == 255) {
    *row = 0;
  } else {
    *row += 1;
  }
  chain = &viewData_->song_->rows_[*row].chains[col];
  uint8_t data = 0xFF;
  if (*chain != 0xFF) {
    data = viewData_->song_->chain_.steps_[*chain][chainPos].phrase;
  }
  return (data != 0xFF);
}

QueueingMode Player::GetQueueingMode(int i) {
  return liveQueueingMode_[i];
}

unsigned char Player::GetQueuePosition(int i) {
  return liveQueuePosition_[i];
}

unsigned char Player::GetQueueChainPosition(int i) {
  return liveQueueChainPosition_[i];
}

void Player::QueueChannel(int i, QueueingMode mode, unsigned char position, unsigned char chainpos) {
  liveQueueingMode_[i] = mode;
  liveQueuePosition_[i] = position;
  liveQueueChainPosition_[i] = chainpos;
}

/************************************************************
 Update:
        this one gets called when the audio driver has just sent
        a block of audio and we get room to prepare the next one
 ************************************************************/

void Player::Update(Observable &o, I_ObservableData *d) {

  // Make sure sync's ok

  MidiService::GetInstance()->Trigger();
  project_->Trigger();

  if (isRunning_) {

    SyncMaster *sync = SyncMaster::GetInstance();
    sync->SetTempo(project_->GetTempo());

    if (!firstPlayCycle_) {
      Groove::GetInstance()->Trigger();
      sync->NextSlice();
      triggerLiveChains_ = false;
      if (retrigAllImmediate_) {
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          QueueChannel(i, QM_TICKSTART, retrigPos_, 0);
        };
        retrigAllImmediate_ = false;
      }
      // Don't advance in audition mode
      if (viewData_->playMode_ != PM_AUDITION)
        moveToNextStep();
      if (triggerLiveChains_) {
        triggerLiveChains();
      };
    }

    // Track which channels just had their delay expire
    bool delayExpired[SONG_CHANNEL_COUNT];
    for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
      delayExpired[i] = false;
      if (timeToStart_[i] > 0) {
        if (--timeToStart_[i] == 0) {
          playCursorPosition(i);
          delayExpired[i] = true;
        }
      }
    }

    // Process commands in current phrase
    if (viewData_->playMode_ != PM_AUDITION)
      ProcessCommands(delayExpired);

    // Initialise retrigger table
    int32_t instrRetrigger[SONG_CHANNEL_COUNT];
    memset(instrRetrigger, -1, SONG_CHANNEL_COUNT * sizeof(int32_t));

    // Process any table commands now
    for (int channel = 0; channel < SONG_CHANNEL_COUNT; channel++) {
      TablePlayback &tpb = TablePlayback::GetTablePlayback(channel);
      if (!tpb.GetAutomation()) {
        TablePlayerChange tpc;
        tpc.timeToLive_ = timeToLive_[channel];
        tpc.instrRetrigger_ = -1;
        tpb.ProcessStep(tpc);
        timeToLive_[channel] = tpc.timeToLive_;
        instrRetrigger[channel] = tpc.instrRetrigger_;
      }
    }

    // Do we need to kill a voice?
    if (sync->TableSlice()) {
      for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
        bool stopped = false;
        if (timeToLive_[i] > 0) {
          if (--timeToLive_[i] == 0) {
            mixer_.StopInstrument(i);
            stopped = true;
          }
        }
        if (!stopped) {
          if (instrRetrigger[i] >= 0) {
            RetriggerChannelInstrument(i, DecodeRetriggerOffset(instrRetrigger[i]), true);
          };
        }
      }
    }

    firstPlayCycle_ = false;
    System *system = System::GetInstance();
    now_ = system->GetClock();

    // Notify refresh

    PlayerEvent pe(PET_UPDATE, 0);
    SetChanged();
    NotifyObservers(&pe);
  }
}

/************************************************************
 ProcessCommands:
        Check if there's any command to trigger at current playing
        position for all channels
 ************************************************************/

void Player::ProcessCommands(bool delayExpired[SONG_CHANNEL_COUNT]) {

  // loop on all channels

  Groove *gs = Groove::GetInstance();

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {

    if (mixer_.IsChannelPlaying(i)) {

      // check if there's any phrase playing

      uint8_t phrase = viewData_->currentPlayPhrase_[i];
      if (phrase != 0xFF) {

        // Skip if waiting for delayed trigger
        if (timeToStart_[i]) {
          continue;
        }

        // If groove says it is time to play OR or the delay just expired
        if (gs->TriggerChannel(i) || (delayExpired && delayExpired[i])) {
          int pos = viewData_->phrasePlayPos_[i];
          const PhraseStep &step = viewData_->song_->phrase_.steps_[phrase][pos];
          Token token = Token::enum_type(step.cmd1);
          uint8_t param = step.param1;

          // if there's any command to trigger, first pass it on the player
          // then pass it on to the instrument

          if (token != Token::InstrumentCommandNone) {
            if (!ProcessChannelCommand(i, token, param)) {
              I_Instrument *instrument = mixer_.GetInstrument(i);
              if (instrument) {
                instrument->ProcessCommand(i, token, param);
              }
            };
          };

          // Now process second command row

          token = Token::enum_type(viewData_->song_->phrase_.steps_[phrase][pos].cmd2);
          param = viewData_->song_->phrase_.steps_[phrase][pos].param2;

          // if there's any command to trigger, first pass it on the player
          // then pass it on to the instrument

          if (token != Token::InstrumentCommandNone) {
            if (!ProcessChannelCommand(i, token, param)) {
              I_Instrument *instrument = mixer_.GetInstrument(i);
              if (instrument) {
                instrument->ProcessCommand(i, token, param);
              }
            };
          };
        }
      }
    }
  };
}

bool Player::ProcessChannelCommand(int channel, Token cmd, uint16_t param) {

  I_Instrument *instr = mixer_.GetInstrument(channel);

  switch (cmd) {
    case Token::InstrumentCommandKill:
      if (instr) {
        int timeToLive = (param & 0xFF);
        timeToLive_[channel] = timeToLive + 1;
      }
      return true;
    case Token::InstrumentCommandTempo:
      {
        param = std::clamp(param, MIN_TEMPO, MAX_TEMPO);
        Variable *v = project_->FindVariable(Token::VarTempo);
        v->SetInt(param);
        SyncMaster *sync = SyncMaster::GetInstance();
        sync->SetTempo(project_->GetTempo());
        return true;
        break;
      }
    case Token::InstrumentCommandTable:
      {
        TableHolder *th = TableHolder::GetInstance();
        TablePlayback &tpb = TablePlayback::GetTablePlayback(channel);
        param = param & 0x7F;
        Table &table = th->GetTable(param);
        tpb.Start(instr, table, false);
        return true;
        break;
      }
    case Token::InstrumentCommandGroove:
      {
        Groove *gr = Groove::GetInstance();
        bool all = (param & 0xFF00) != 0;
        param = param & 0xFF;
        if (all) {
          for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
            gr->SetGroove(i, param);
          }
        } else {
          gr->SetGroove(channel, param);
        }
      }
      break;
    default:
      break;
  };
  return false;
}

/********************************************************
 triggerLiveChains:
        look if there's any chain needed to be started
                i.e. they're queued but on a channel that is not
                yet active
 ********************************************************/

void Player::triggerLiveChains() {

  if (mode_ == PM_LIVE) {
    for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
      if (!(mixer_.IsChannelPlaying(i)) &&
          ((liveQueueingMode_[i] == QM_CHAINSTART) || (liveQueueingMode_[i] == QM_TICKSTART) ||
           (liveQueueingMode_[i] == QM_PHRASESTART))) {
        if (findPlayable(&(liveQueuePosition_[i]), i, liveQueueChainPosition_[i])) {
          mixer_.StartChannel(i);
          updateSongPos(liveQueuePosition_[i], i, liveQueueChainPosition_[i]);
        }
        liveQueueingMode_[i] = QM_NONE;
      }
    }
  }
}

/********************************************************
 updateSongPos:
        sets current song position for a give channel. the
        last parameter allow to start the song position at a
        give chain position directly (for PM_CHAIN/PM_PHRASE)
 ********************************************************/

void Player::updateSongPos(int pos, int channel, int chainPos, int hop) {
  unsigned char *data = &viewData_->song_->rows_[pos].chains[channel];
  viewData_->songPlayPos_[channel] = pos;
  viewData_->currentPlayChain_[channel] = *data;
  updateChainPos(chainPos, channel, hop);
}

/********************************************************
 updateChainPos:
        sets current chain position for a give channel
 ********************************************************/

void Player::updateChainPos(int pos, int channel, int hop) {
  unsigned char chain = viewData_->currentPlayChain_[channel];
  if (chain != 0xFF) {
    viewData_->chainPlayPos_[channel] = pos;
    viewData_->currentPlayPhrase_[channel] = viewData_->song_->chain_.steps_[chain][pos].phrase;
    if (viewData_->currentPlayPhrase_[channel] == EMPTY_CHAIN_VALUE) {
      // This could happen if starting in song mode on a row where a chain contains no phrase
      mixer_.StopChannel(channel);
    }
  } else {
    viewData_->currentPlayPhrase_[channel] = EMPTY_CHAIN_VALUE;
    mixer_.StopChannel(channel);
  };
  updatePhrasePos((hop >= 0) ? hop : 0, channel);
}

/********************************************************
 updatePhrasePos:
        sets current phrase position for a give channel.
 ********************************************************/

void Player::updatePhrasePos(int pos, int channel) {

  viewData_->phrasePlayPos_[channel] = pos;

  // See if we need to delay the trigger
  timeToStart_[channel] = 1;

  uint8_t phrase = viewData_->currentPlayPhrase_[channel];

  // Check both param colum 1 & 2

  Token token = Token::enum_type(viewData_->song_->phrase_.steps_[phrase][pos].cmd1);
  if (token == Token::InstrumentCommandDelay) {
    uint8_t param = viewData_->song_->phrase_.steps_[phrase][pos].param1;
    timeToStart_[channel] = (param & 0x0F) + 1;
  }

  token = Token::enum_type(viewData_->song_->phrase_.steps_[phrase][pos].cmd2);
  if (token == Token::InstrumentCommandDelay) {
    uint8_t param = viewData_->song_->phrase_.steps_[phrase][pos].param2;
    timeToStart_[channel] = (param & 0x0F) + 1;
  }
}

void Player::playCursorPosition(int channel) {
  int pos = viewData_->phrasePlayPos_[channel];
  bool noteTriggered = false;

  // Get chain content and see if instr needs to be started/Stopped
  unsigned char currentPhrase = viewData_->currentPlayPhrase_[channel];

  if (currentPhrase == EMPTY_CHAIN_VALUE) {
    return;
  }

  Song *song = viewData_->song_;
  Phrase *phrase = &(song->phrase_);
  unsigned char note = phrase->steps_[currentPhrase][pos].note;
  unsigned char instr = phrase->steps_[currentPhrase][pos].instrument;
  uint8_t stepVolume = phrase->steps_[currentPhrase][pos].volume;

  TableHolder *th = TableHolder::GetInstance();
  TablePlayback &tpb = TablePlayback::GetTablePlayback(channel);
  TablePlayback &atp = TablePlayback::GetAutomationPlayback(channel);

  // note off ********************************************************************************************************
  if (note == NOTE_OFF) {
    mixer_.StopInstrument(channel);
  } else if (note <= HIGHEST_NOTE) {
    // note on *******************************************************************************************************

    // Stop instrument if playing
    mixer_.StopInstrument(channel, true);
    InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

    // get instrument for next note
    bool newInstrument = false;

    I_Instrument *instrument;

    if (instr != 0xFF) {
      instrument = bank->GetInstrument(instr);
      newInstrument = true;
    } else {
      instrument = mixer_.GetLastInstrument(channel);
    }

    if (instrument == NULL) {
      instrument = bank->GetInstrument(0);
    }

    if (instrument != 0) {
      int chain = viewData_->currentPlayChain_[channel];
      int chainPos = viewData_->chainPlayPos_[channel];
      bool preserveNote = false;
      // TODO: this is special handling that should not happen here
      if (instrument->GetType() == IT_SAMPLE) {
        SampleInstrument *sampleInstrument = static_cast<SampleInstrument *>(instrument);
        preserveNote = sampleInstrument->HasSlicesForPlayback();
      }
      if (!preserveNote) {
        note += viewData_->song_->chain_.steps_[chain][chainPos].transpose;
        note += project_->GetTranspose();
      }
      instrumentOnChannel_[channel] = instr;

      // Check if note is in acceptable midi range

      if (note < HIGHEST_PLAYABLE_NOTE) {
        mixer_.StartInstrument(channel, instrument, note, stepVolume, newInstrument);
        noteTriggered = true;
        int instrTable = instrument->GetTable();

        // If an instrument number has been specified && instrument has table,
        // we trigger the table.

        if ((instrTable != VAR_OFF) && (newInstrument)) {
          Table &table = th->GetTable(instrTable);
          bool automated = instrument->GetTableAutomation();
          if (automated) {
            atp.Start(instrument, table, true);
            tpb.Stop();
          } else {
            atp.Stop();
            tpb.Start(instrument, table, false);
          }
        } else {
          // if there was an instrument number, we stop the table
          if (newInstrument) {
            atp.Stop();
            tpb.Stop();
          }
        }
      } else {
        Trace::Error("Note outside range: %02x", (unsigned int)note);
      }
    }
  }

  if ((note <= HIGHEST_NOTE) || (instr != 0xFF)) {
    I_Instrument *instrument = mixer_.GetInstrument(channel);
    if (instrument) {
      if (instrument->GetTableAutomation()) {
        TablePlayerChange tpc;
        tpc.timeToLive_ = timeToLive_[channel];
        tpc.instrRetrigger_ = -1;
        atp.ProcessStep(tpc);
        timeToLive_[channel] = tpc.timeToLive_;
        if (tpc.instrRetrigger_ >= 0) {
          RetriggerChannelInstrument(channel, DecodeRetriggerOffset(tpc.instrRetrigger_), false);
          noteTriggered = true;
        }
      }
    }
  }

  if (!noteTriggered && stepVolume != NO_VOLUME) {
    I_Instrument *instrument = mixer_.GetInstrument(channel);

    if (instrument) {
      instrument->SetStepVolume(channel, stepVolume);
    }
  }
}

void Player::StepAutomationTableForRetrigger(int channel, I_Instrument *instrument) {
  if ((instrument == 0) || (!instrument->GetTableAutomation())) {
    return;
  }

  int instrTable = instrument->GetTable();
  if (instrTable == VAR_OFF) {
    return;
  }

  TableHolder *th = TableHolder::GetInstance();
  TablePlayback &automationPlayback = TablePlayback::GetAutomationPlayback(channel);
  TablePlayerChange tpc;

  Table &table = th->GetTable(instrTable);
  if (automationPlayback.GetTable() != &table) {
    automationPlayback.Start(instrument, table, true);
  }

  tpc.timeToLive_ = timeToLive_[channel];
  tpc.instrRetrigger_ = -1;

  automationPlayback.ProcessStep(tpc);

  timeToLive_[channel] = tpc.timeToLive_;
  // Ignore IRT generated by this follow-up automation step to keep retrigger
  // handling strictly non-recursive.
}

void Player::RetriggerChannelInstrument(int channel, int semitoneOffset, bool stepAutomationTable) {
  int note = mixer_.GetChannelNote(channel);
  uint8_t volume = mixer_.GetChannelVolume(channel);
  I_Instrument *instrument = mixer_.GetInstrument(channel);

  if ((note > HIGHEST_NOTE) || (instrument == 0)) {
    return;
  }

  note += semitoneOffset;
  while (note > HIGHEST_NOTE) {
    note -= 12;
  }
  while (note < 0) {
    note += 12;
  }

  mixer_.StopInstrument(channel);
  // NOTE: 'newInstrument' bool flag is really the "retrigger" flag
  mixer_.StartInstrument(channel, instrument, note, volume, false);

  if (stepAutomationTable) {
    StepAutomationTableForRetrigger(channel, instrument);
  }
}

int Player::getChannelHop(int channel, int pos) {

  int phrase = viewData_->currentPlayPhrase_[channel];
  Token token = Token::enum_type(viewData_->song_->phrase_.steps_[phrase][pos].cmd1);
  if (token == Token::InstrumentCommandHop) {
    return (viewData_->song_->phrase_.steps_[phrase][pos].param1) & 0xF;
  }
  token = Token::enum_type(viewData_->song_->phrase_.steps_[phrase][pos].cmd2);
  if (token == Token::InstrumentCommandHop) {
    return (viewData_->song_->phrase_.steps_[phrase][pos].param2) & 0xF;
  }
  return -1;
}

/********************************************************
 moveToNextStep:
        Atomic routine that triggers the next step for all
        playing channels.
 ********************************************************/

void Player::moveToNextStep() {
  // we'll need to know if any channel is playing

  bool playingChannel = false;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    bool liveTriggered = false;

    switch (liveQueueingMode_[i]) {
      case QM_TICKSTART:
        liveQueueingMode_[i] = QM_NONE;
        if (findPlayable(&(liveQueuePosition_[i]), i, liveQueueChainPosition_[i])) {
          liveTriggered = true;
          updateSongPos(liveQueuePosition_[i], i, liveQueueChainPosition_[i]);
        }
        return;
        break;
      case QM_PHRASESTART:
      case QM_PHRASESTOP:
      case QM_CHAINSTART:
      case QM_CHAINSTOP:
      case QM_NONE:
        break;
    }

    Groove *gs = Groove::GetInstance();

    if (mixer_.IsChannelPlaying(i) && !liveTriggered) {
      playingChannel = true;

      if (gs->TriggerChannel(i)) { // If groove says it is time to play
        if (viewData_->currentPlayPhrase_[i] != 0xFF) {
          int pos = (viewData_->phrasePlayPos_[i]) + 1;
          if (pos != 16) {
            int hop = getChannelHop(i, pos);
            if (hop >= 0) {
              if (mode_ != PM_PHRASE) {
                moveToNextPhrase(i, hop);
              } else {
                updatePhrasePos(hop, i);
              }
            } else {
              updatePhrasePos(pos, i);
            }
          } else { // HOP.. something should be done so that if
                   // next chain has a hop on pos zero, it is effective
            if (mode_ != PM_PHRASE) {
              moveToNextPhrase(i);
            } else {
              updatePhrasePos(0, i);
            }
          }
        }
      }
    }
  }
  // if no channel is playing we allow straight
  // queueing of chains
  if (!playingChannel) {
    triggerLiveChains_ = true;
  };
}

/********************************************************
 moveToNextPhrase:
        Compute what is the next phrase to be triggered on
        the selected channel. Called when the current phrase
        has reached the end
 ********************************************************/

void Player::moveToNextPhrase(int channel, int hop) {

  // First check if we're in live mode and the channel
  // has been trigged in immediate mode. In which case we
  // do the action straight away (START/STOP)

  if (mode_ == PM_LIVE) {

    switch (liveQueueingMode_[channel]) {
      case QM_TICKSTART:
      case QM_PHRASESTART:
        if (findPlayable(&(liveQueuePosition_[channel]), channel, liveQueueChainPosition_[channel])) {
          updateSongPos(liveQueuePosition_[channel], channel, liveQueueChainPosition_[channel], hop);
        }
        liveQueueingMode_[channel] = QM_NONE;
        return;
        break;
      case QM_PHRASESTOP:
        mixer_.StopChannel(channel);
        liveQueueingMode_[channel] = QM_NONE;
        return;
      case QM_CHAINSTART:
      case QM_CHAINSTOP:
      case QM_NONE:
        break;
    }
  }

  // If nothing has been triggered, we need to find what is
  // The next phrase to play

  int chain = viewData_->currentPlayChain_[channel];
  int pos = (viewData_->chainPlayPos_[channel]) + 1;

  // Look if there' any data at current position
  // which means we continue in the current chain

  bool canContinue = (pos < 16);
  if (canContinue) {
    canContinue = (viewData_->song_->chain_.steps_[chain][pos].phrase != 0xFF);
  }

  // If so, we trigger it. Otherwise, we go to the next phrase

  if (canContinue) {
    updateChainPos(pos, channel, hop);
  } else { // Should move to next chain
    if ((mode_ == PM_SONG) || (mode_ == PM_LIVE)) {
      moveToNextChain(channel, hop); // HOP. here
    } else {
      updateChainPos(0, channel, hop);
    };
  }
}

/********************************************************
 moveToNextChain:
        Compute what is the next chain to be triggered on
        the selected channel. Called when the current chain
        has reached the end.
 ********************************************************/

void Player::moveToNextChain(int channel, int hop) {

  // if there's unplaying channels queue they should be started
  // once all position have been updated

  triggerLiveChains_ = true;

  bool searchNext = true;
  int nextPos = 0;
  int chainPosition = 0;

  // Hop here ?

  // See if current channel has been queued to play something
  // in normal mode

  if (mode_ == PM_LIVE) {
    switch (liveQueueingMode_[channel]) {

      case QM_CHAINSTART:
      case QM_PHRASESTART:

        if (findPlayable(&(liveQueuePosition_[channel]), channel, liveQueueChainPosition_[channel])) {
          nextPos = liveQueuePosition_[channel];
          searchNext = false;
          liveQueueingMode_[channel] = QM_NONE;
          chainPosition = liveQueueChainPosition_[channel];
        } else {
          liveQueueingMode_[channel] = QM_NONE;
        }
        break;

      case QM_CHAINSTOP:
      case QM_PHRASESTOP:
        mixer_.StopChannel(channel);
        liveQueueingMode_[channel] = QM_NONE;
        return;

      case QM_TICKSTART:
      case QM_NONE:
        break;
    }
  }

  // if live mode didn't queue anything, we find the next to play

  if (searchNext) {
    int pos = (viewData_->songPlayPos_[channel]) + 1;
    uint8_t chainId = viewData_->song_->rows_[pos].chains[channel];
    bool loopBack = (chainId == EMPTY_SONG_VALUE);
    // Check if first step of chain contains somethin, if not we loop back
    if (!loopBack) {
      unsigned char phraseId = viewData_->song_->chain_.steps_[chainId][0].phrase;
      loopBack = (phraseId == EMPTY_CHAIN_VALUE);
    };
    if (loopBack) {
      // If stopAtEnd is enabled and we need to loop back, stop playback instead
      if (stopAtEnd_) {
        // Stop all channels
        for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
          mixer_.StopChannel(i);
        }
        MidiService::GetInstance()->OnPlayerStop();
        mixer_.OnPlayerStop();

        isRunning_ = false;
        SetChanged();
        PlayerEvent pe(PET_STOP);
        NotifyObservers(&pe);
        // We're already inside a locked context, so we don't need to call
        // Stop() as that will cause a deadlock
        return;
      }

      // Otherwise proceed with normal loop back behavior
      pos--;
      while (pos >= 0) {
        chainId = viewData_->song_->rows_[pos].chains[channel];

        if (chainId == EMPTY_SONG_VALUE) {
          // we stop searching if there's a blank
          break;
        } else {
          // Or if first phrase of chain is empty
          if (viewData_->song_->chain_.steps_[chainId][0].phrase == EMPTY_CHAIN_VALUE) {
            break;
          }
        }
        pos--;
      }

      pos++;
    }

    nextPos = pos;
  }
  // Do a last check in case we had only one chain and it go destroyed

  if (isPlayable(nextPos, channel, chainPosition)) {
    updateSongPos(nextPos, channel, chainPosition, hop);
  } else {
    mixer_.StopChannel(channel);
  }
}

double Player::GetPlayTime() {
  AudioOut *out = mixer_.GetAudioOut();
  double currentTime = out->GetStreamTime();
  return currentTime - startTime_;
}

int Player::GetPlayedBufferPercentage() {
  unsigned int beatCount = SyncMaster::GetInstance()->GetBeatCount();
  if (beatCount != lastBeatCount_) {
    lastBeatCount_ = beatCount;
    lastPercentage_ = mixer_.GetPlayedBufferPercentage();
  }
  return lastPercentage_;
}

PlayerEvent::PlayerEvent(PlayerEventType type, unsigned int tickCount) : ViewEvent(VET_PLAYER_POSITION_UPDATE) {
  type_ = type;
  tickCount_ = tickCount;
}

PlayerEventType PlayerEvent::GetType() {
  return type_;
}

unsigned int PlayerEvent::GetTickCount() {
  return tickCount_;
}

void Player::StartStreaming(const char *name, int startSample) {
  mixer_.StartStreaming(name, startSample);
  if (!isRunning_) {
    SetAudioActive(true);
  }
}

void Player::StartLoopingStreaming(const char *name) {
  mixer_.StartLoopingStreaming(name);
  if (!isRunning_) {
    SetAudioActive(true);
  }
}

void Player::StopStreaming() {
  mixer_.StopStreaming();
  if (!isRunning_) {
    SetAudioActive(false);
  }
}

void Player::SetAudioActive(bool active) {
  MixerService *ms = MixerService::GetInstance();
  AudioOut *out = (ms != nullptr) ? ms->GetAudioOut() : nullptr;
  if (out) {
    out->SetAudioActive(active);
  }
}

bool Player::IsPlaying() {
  return mixer_.IsPlaying();
}

etl::string<STRING_AUDIO_API_MAX> Player::GetAudioAPI() {
  AudioOut *out = mixer_.GetAudioOut();
  return (out) ? out->GetAudioAPI() : "";
}

etl::string<STRING_AUDIO_DEVICE_MAX> Player::GetAudioDevice() {
  AudioOut *out = mixer_.GetAudioOut();
  return (out) ? out->GetAudioDevice() : "";
}

int Player::GetAudioBufferSize() {
  AudioOut *out = mixer_.GetAudioOut();
  return (out) ? out->GetAudioBufferSize() : 0;
}

int Player::GetAudioRequestedBufferSize() {
  AudioOut *out = mixer_.GetAudioOut();
  return (out) ? out->GetAudioRequestedBufferSize() : 0;
}

int Player::GetAudioPreBufferCount() {
  AudioOut *out = mixer_.GetAudioOut();
  return (out) ? out->GetAudioPreBufferCount() : 0;
}

etl::array<stereosample, SONG_CHANNEL_COUNT> *Player::GetMixerLevels() {
  return mixer_.GetMixerLevels();
}

// Direct note playback methods for MIDI

void Player::PlayNote(uint16_t instrumentIndex, uint16_t channel, unsigned char note, unsigned char velocity) {
  if (!project_) {
    return;
  }

  InstrumentBank *bank = project_->GetInstrumentBank();
  if (!bank) {
    return;
  }

  I_Instrument *instrument = bank->GetInstrument(instrumentIndex);
  if (instrument) {
    // Use the channel modulo SONG_CHANNEL_COUNT to ensure it's within range
    int playerChannel = channel % SONG_CHANNEL_COUNT;
    mixer_.StartInstrument(playerChannel, instrument, note, velocity, true);
    if (!isRunning_) {
      SetAudioActive(true);
    }
  }
}

void Player::StopNote(uint16_t instrumentIndex, uint16_t channel) {
  // Use the channel modulo SONG_CHANNEL_COUNT to ensure it's within range
  int playerChannel = channel % SONG_CHANNEL_COUNT;
  mixer_.StopInstrument(playerChannel);
  if (!isRunning_) {
    SetAudioActive(false);
  }
}
