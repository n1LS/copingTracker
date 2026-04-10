git ls-files "*.c" "*.cpp" "*.h" \
  | grep -v "^sources/Externals/" \
  | xargs clang-format --style=file -i