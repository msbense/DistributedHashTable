git pull
if [ "$#" -eq 1 ]; then
    make $1
fi

if [ "$#" -eq 0 ]; then
    make
fi