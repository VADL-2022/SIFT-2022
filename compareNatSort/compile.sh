if [[ main.cpp -nt compareNatSort ]]; then
    ../tools/compileSubproject.sh main.cpp -o compareNatSort
fi
./compareNatSort . ".cpp" | grep main # tests if it works based on exit code
