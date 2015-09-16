#!/bin/sh
GIT_SHA1=`(git show-ref --head --hash=8 2> /dev/null || echo 00000000) | head -n1`
GIT_DIRTY=`git diff --no-ext-diff 2> /dev/null | wc -l`
SERVER_ID=`uname -n`"-"`date +%s`
test -f gent_hw.h || touch gent_hw.h
(cat gent_hw.h | grep SERVER_ID) && exit 0
(cat gent_hw.h | grep SHA1 | grep $GIT_SHA1) && \
(cat gent_hw.h | grep DIRTY | grep $GIT_DIRTY) && exit 0
echo "#define GIT_SHA1 \"$GIT_SHA1\"" > gent_hw.h
echo "#define GIT_DIRTY \"$GIT_DIRTY\"" >> gent_hw.h
echo "const std::string SERVER_ID=\"$SERVER_ID\";" >> gent_hw.h
