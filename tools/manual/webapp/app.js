(function () {
  "use strict";

  var DATA = JSON.parse(document.getElementById("manual-data").textContent);

  var COLS = DATA.columns;
  var GW = DATA.glyphWidth;
  var GH = DATA.glyphHeight;

  var canvas = document.getElementById("canvas");
  var ctx = canvas.getContext("2d", { alpha: false });
  var textlayer = document.getElementById("textlayer");
  var tabs = document.getElementById("tabs");
  var zoom = document.getElementById("zoom");
  var zoomLabel = document.getElementById("zoomLabel");

  // Decode the base64 glyph atlas: 2 bytes per glyph row, low byte first.
  var atlas = (function (b64) {
    var bin = atob(b64);
    var out = new Uint16Array(bin.length / 2);
    for (var i = 0; i < out.length; i++) {
      out[i] = bin.charCodeAt(i * 2) | (bin.charCodeAt(i * 2 + 1) << 8);
    }
    return out;
  })(DATA.atlas);

  // Palette as packed RGB, so pixels can be written straight into ImageData.
  var palette = DATA.palette.map(function (hex) {
    return [
      parseInt(hex.substr(1, 2), 16),
      parseInt(hex.substr(3, 2), 16),
      parseInt(hex.substr(5, 2), 16)
    ];
  });

  var current = 0;
  var scale = parseInt(zoom.value, 10);

  var FONT_STACK = 'ui-monospace, SFMono-Regular, Menlo, Consolas, "Courier New", monospace';
  var PROBE_TEXT = "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMM";

  // Only codes that map to real letters get a selectable character; the box
  // drawing and button glyphs have no sensible text equivalent, so they
  // become spaces rather than mojibake in a copy/paste.
  function toText(code) {
    return code >= 32 && code < 127 ? String.fromCharCode(code) : " ";
  }

  function render() {
    var page = DATA.pages[current];
    var rows = page.rows;

    // Draw at 1x into an offscreen buffer, then blit scaled with smoothing
    // off. Keeps the glyphs pixel exact at every zoom level.
    var w = COLS * GW;
    var h = rows * GH;

    var img = ctx.createImageData(w, h);
    var px = img.data;

    for (var cell = 0, n = rows * COLS; cell < n; cell++) {
      var color = parseInt(page.colors.substr(cell * 2, 2), 16);
      var fg = palette[(color >> 4) & 0xf];
      var bg = palette[color & 0xf];
      var code = page.chars[cell];

      var cx = (cell % COLS) * GW;
      var cy = ((cell / COLS) | 0) * GH;
      var glyph = code * GH;

      for (var y = 0; y < GH; y++) {
        var bits = atlas[glyph + y];
        var rowBase = ((cy + y) * w + cx) * 4;

        for (var x = 0; x < GW; x++) {
          var on = (bits >> (GW - 1 - x)) & 1;
          var c = on ? fg : bg;
          var p = rowBase + x * 4;
          px[p] = c[0];
          px[p + 1] = c[1];
          px[p + 2] = c[2];
          px[p + 3] = 255;
        }
      }
    }

    var buffer = document.createElement("canvas");
    buffer.width = w;
    buffer.height = h;
    buffer.getContext("2d").putImageData(img, 0, 0);

    var dpr = window.devicePixelRatio || 1;
    canvas.width = w * scale * dpr;
    canvas.height = h * scale * dpr;
    canvas.style.width = w * scale + "px";
    canvas.style.height = h * scale + "px";

    ctx.imageSmoothingEnabled = false;
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.drawImage(buffer, 0, 0, canvas.width, canvas.height);

    // Build the invisible selection layer with the exact same cell metrics
    // so a drag selects the characters the user sees under the cursor.
    var lines = [];
    for (var r = 0; r < rows; r++) {
      var line = "";
      for (var c2 = 0; c2 < COLS; c2++) {
        line += toText(page.chars[r * COLS + c2]);
      }
      // Trailing blanks would extend the selection past the visible text.
      lines.push(line.replace(/\s+$/, ""));
    }

    textlayer.textContent = lines.join("\n");
    textlayer.style.width = w * scale + "px";
    textlayer.style.height = h * scale + "px";
    alignTextLayer();
  }

  // The selection rectangle only lines up with the drawn grid if one text
  // character advances by exactly one cell. Rather than assume a metric for
  // whatever monospace font the browser picked, measure it and close the gap
  // with letter-spacing.
  function alignTextLayer() {
    var cellW = GW * scale;
    var cellH = GH * scale;
    // An even font size keeps the browser's half-leading from rounding the
    // baseline a pixel off on odd zoom levels.
    var fontSize = Math.max(2, Math.round(cellH * 0.8 / 2) * 2);

    textlayer.style.fontFamily = FONT_STACK;
    textlayer.style.fontSize = fontSize + "px";
    textlayer.style.lineHeight = cellH + "px";
    textlayer.style.letterSpacing = "0px";

    // The selection highlight is sized from the font's box rather than the
    // line box, so scale the font until that box is exactly one cell tall.
    var probeH = document.createElement("span");
    probeH.style.position = "absolute";
    probeH.style.visibility = "hidden";
    probeH.style.fontFamily = FONT_STACK;
    probeH.style.fontSize = fontSize + "px";
    probeH.style.lineHeight = "normal";
    probeH.textContent = "M";
    document.body.appendChild(probeH);
    var inkH = probeH.getBoundingClientRect().height;
    if (inkH > 0) {
      fontSize = Math.max(1, fontSize * cellH / inkH);
      probeH.style.fontSize = fontSize + "px";
    }
    document.body.removeChild(probeH);

    textlayer.style.fontSize = fontSize + "px";

    var probe = document.createElement("span");
    probe.style.position = "absolute";
    probe.style.visibility = "hidden";
    probe.style.whiteSpace = "pre";
    probe.style.fontFamily = FONT_STACK;
    probe.style.fontSize = fontSize + "px";
    probe.style.letterSpacing = "0px";
    probe.textContent = PROBE_TEXT;
    document.body.appendChild(probe);
    var advance = probe.getBoundingClientRect().width / PROBE_TEXT.length;
    document.body.removeChild(probe);

    textlayer.style.letterSpacing = (cellW - advance) + "px";
  }

  function selectTab(index) {
    current = index;
    Array.prototype.forEach.call(tabs.children, function (b, i) {
      b.setAttribute("aria-selected", i === index ? "true" : "false");
    });
    render();
  }

  DATA.pages.forEach(function (page, i) {
    var b = document.createElement("button");
    b.type = "button";
    b.textContent = page.title;
    b.setAttribute("role", "tab");
    b.addEventListener("click", function () { selectTab(i); });
    tabs.appendChild(b);
  });

  zoom.addEventListener("input", function () {
    scale = parseInt(zoom.value, 10);
    zoomLabel.textContent = scale + "x";
    render();
  });

  document.addEventListener("keydown", function (e) {
    if (e.key === "ArrowLeft" && current > 0) selectTab(current - 1);
    if (e.key === "ArrowRight" && current < DATA.pages.length - 1) selectTab(current + 1);
  });

  selectTab(0);
})();
