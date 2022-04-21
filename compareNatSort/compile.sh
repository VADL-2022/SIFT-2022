if [[ main.cpp -nt compareNatSort ]]; then
    echo "Recompiling compareNatSort"
    ../tools/compileSubproject.sh main.cpp -o compareNatSort
else
    echo "compareNatSort is up-to-date"
fi
./compareNatSort . ".cpp" | grep main # tests if it works based on exit code
echo "compareNatSort and grep exit code: $?"
