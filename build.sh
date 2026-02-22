#!/bin/sh

echo "Build not implemented yet... (todo)"

if [[ "$(uname)" == "Linux"]]; then
  echo "Building for Linux... (unsupported)"  
elif [[ "$(uname)" == "MacOS"]]; then
  echo "Building for MacOS... (unsupported)"
else
  echo "Building for Windows... (supported)"
  libs="-luser32"
  includes="-Isrc"
  outputFileServer="server.exe"
  outputFileClient="client.exe"
fi