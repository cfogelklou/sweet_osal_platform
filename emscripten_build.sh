rm -rf em
mkdir em && cd em
emcmake cmake ..
#make clean
emmake make -j8

# Run node unit tests on all generated .js files.
find . -name "*.js" -exec node {} \;


