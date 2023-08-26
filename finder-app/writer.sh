writefile=$1
writestr=$2

writefile="${writefile#/}"

if [ -z "$writefile" ]; then
    echo "path to file is missing"
    exit 1
else
    if [ -z "$writestr" ]; then
        echo "String to write is missing"
        exit 1
    else
        d_name="$(dirname "${writefile}")"

        if [ ! -d "$d_name" ]; then
            mkdir -p "$d_name"
        fi

        echo $writestr > $writefile
    fi
fi