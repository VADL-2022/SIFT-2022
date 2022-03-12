if [ -z "$1" ]; then
    echo "Need an argument. Exiting."
    exit 1
fi

# https://superuser.com/questions/1220975/os-x-remove-forward-slash-from-filename
for f in "$1"/*; do
dir="$(dirname "$f")"
file="$(basename "$f")"
#echo mv -- "$f" "${dir}/${file//[^_-0-9A-Za-z.]}"
mv -- "$f" "${dir}/${file//[:\/]/_}" # Regex substitution of `:` or `/` with `_`
done
