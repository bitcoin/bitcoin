#!/bin/bash
for file in $(git ls-files *.c *.h *.in *.s *.S *.tmpl)
do
    HASH=$(git rev-list HEAD "$file" | tail -n 1)
    DATE=$(git show -s --format="%ad" --date=format:'%Y' $HASH --)
    printf "%-35s %s\n  %s\n" "$file" "$DATE"
    sed -i "s/Copyright (c) 20[0-9][0-9]/Copyright (c) $DATE/g" "$file"
done


