#!/bin/bash
touch rn_lib/rn-pak-ble/android/CMakeLists.txt
watchman watch-del-all
npm start -- --reset-cache

