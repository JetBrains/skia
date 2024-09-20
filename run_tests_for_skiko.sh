# running locally? do not forget to install ninja - python3 bin/fetch-ninja

bin/gn gen out/Debug
./third_party/ninja/ninja -C out/Debug dm
out/Debug/dm -v --match Skiko