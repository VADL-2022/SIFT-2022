if [[ main.cpp -nt compareNatSort ]]; then
    ../tools/compileSubproject.sh main.cpp -o compareNatSort
fi
compareNatSort
