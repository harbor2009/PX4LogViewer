#!/bin/sh
dir=${PWD##*/}
time=`date +%Y%m%d%H%M%S`
user=`hostname`
tar --exclude='*.pro.user*' --exclude='build-*' --exclude='.*' --exclude-vcs -acf ../$dir.$time.$user.tar.xz *
