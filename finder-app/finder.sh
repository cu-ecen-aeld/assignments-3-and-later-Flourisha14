filesdir=$1
searchstr=$2
numberoffiles=0
matchingstrings=0

if [ -z "$filesdir" ]; then
    echo "path to a directory is missing"
    exit 1
else
    if [ ! -d "$filesdir" ]; then
        echo "${filesdir} is not a directory"
        exit 1
    else
        if [ -z "$searchstr" ]; then
            echo "Search string is missing"
            exit 1
        else
            numberoffiles=$(find "$filesdir" -type f | wc -l)
            matchingstrings=$(grep -R $searchstr $filesdir | wc -l)
            echo "The number of files are ${numberoffiles} and the number of matching lines are ${matchingstrings}"
        fi
    fi
fi