#!/bin/sh

# Set up AutoTools directory structure.
# Pretty blunt - just deletes whatever is there and starts over

Dev=Development
Rel=Release

curdir=`pwd`

if test -d $HOME/$Rel/AstroCTC
then
    rm -rf $HOME/$Rel/AstroCTC
fi

mkdir -p $HOME/$Rel/AstroCTC
cd $HOME/$Rel/AstroCTC
mkdir src
mkdir -p data/icons

ln -s $HOME/$Dev/AstroCTC/README .
ln -s $HOME/$Dev/AstroCTC/COPYING .
ln -s $HOME/$Dev/AstroCTC/AutoTools/AUTHORS .
ln -s $HOME/$Dev/AstroCTC/AutoTools/ChangeLog .
ln -s $HOME/$Dev/AstroCTC/AutoTools/INSTALL .
ln -s $HOME/$Dev/AstroCTC/AutoTools/NEWS .
ln -s $HOME/$Dev/AstroCTC/AutoTools/configure.ac .
ln -s $HOME/$Dev/AstroCTC/AutoTools/Makefile.am .

cd src
ln -s $HOME/$Dev/AstroCTC/AutoTools/src/Makefile.am .
cp -p $HOME/$Dev/AstroCTC/src/*.c .
cp -p $HOME/$Dev/AstroCTC/src/*.h .

cd ../data
ln -s $HOME/$Dev/AstroCTC/AutoTools/data/astroctc.1 .
ln -s $HOME/$Dev/AstroCTC/AutoTools/data/astroctc.desktop .
ln -s $HOME/$Dev/AstroCTC/AutoTools/data/Makefile.am .

cd icons
ln -s $HOME/$Dev/AstroCTC/AutoTools/data/icons/astroctc.png .
ln -s $HOME/$Dev/AstroCTC/AutoTools/data/icons/Makefile.am .

cd ../..

autoreconf -i

cd $curdir

exit 0
