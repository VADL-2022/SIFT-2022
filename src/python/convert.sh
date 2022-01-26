git clone --recursive https://github.com/VADL-2022/IMU_VV
for d in IMU_VV/*/ ; do
    #echo "$d"
    #shopt -s nullglob # https://superuser.com/questions/519374/how-to-handle-bash-matching-when-there-are-no-matches
    #echo "$d/"*.ipynb | xargs -I {} -n 1 echo jupyter nbconvert {} --to python --output {}.py
    #f="$d/"*.ipynb
    #echo "$f"
    for f in "$d/"*.ipynb
    do
	[ -e "$f" ] || continue # https://superuser.com/questions/519374/how-to-handle-bash-matching-when-there-are-no-matches
	#if [ ! -z $f ]; then # https://unix.stackexchange.com/questions/314804/how-to-make-bash-glob-a-string-variable : "Globbing happens after variable expansion, if the variable is unquoted"
	jupyter nbconvert "$f" --to python --output-dir=.
	#fi
    done
    #exit
done
