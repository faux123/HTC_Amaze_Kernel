#!/bin/bash


#GLOBALS
	TI_COMPAT_DIR=$(pwd)
	NEW_REJ_DIR=rejects-new
	ORIG_REJ_DIR=rejects-orig
	ADMIN_UP_RES_DIR=admin-upd-res
	COMPAT_WIRELESS_REJ=comp-wire-rej
	TI_REJ=ti-rej
	COMPAT_WIRELESS_ORIG_REJ_TAR=comp-wire-rej-org.tar
	COMPAT_WIRELESS_ORIG_REJ=comp-wire-rej-org
	TI_COMPAT_PATCH=$TI_COMPAT_DIR/TI-compat.patch
	LAST_GOOD_DIR=last-good-dir
	CREATE_PATCH_PARAM=create-patch
    MAKE_ONLY_PARAM=make-only
	GATHER_ONLY_PARAM=gather-only
#FUNCTIONS


function echo_red() {
	echo -en '\E[00;31m'"\033[1m$*\033[0m"
	echo
}

function echo_green() {
	echo -en '\E[00;32m'"\033[1m$*\033[0m"
	echo
}

function echo_yellow() {
	echo -en '\E[00;33m'"\033[1m$*\033[0m"
	echo
}

function echo_blue() {
	echo -en '\E[00;34m'"\033[1m$*\033[0m"
	echo
}

function echo_lblue() {
	echo -en '\E[00;36m'"\033[1m$*\033[0m"
	echo
}

function echo_bold() {
	echo -en '\E[00;38m'"\033[1m$*\033[0m"
	echo
}


# moves rej files to a given dir
# param 1 - target dir name, it will be overwritten
function build_rej_dir() {
	rm -fR "$1"
	mkdir "$1" && find . -name "*.rej"  | grep -v "$ORIG_REJ_DIR" | cpio -pvdmu ./"$1"/
	return $?
}



#preparation phase
function praparation_phase(){
	echo "Running preparation phase..."

	#run admin-refresh.sh
	echo "Running admin-clean script..."
	./scripts/admin-clean.sh
	#manualy remove sources ( just in case )
	rm -fR net
	rm -fR drivers
	rm -fR include
	echo "Running admin-update script..."
	./scripts/admin-update.sh

	#move rejects out of the way and wipe out tmp dirs and orig files
	rm -fR "$COMPAT_WIRELESS_REJ"
	rm -fR "$TI_REJ"
	rm -fR "$COMPAT_WIRELESS_ORIG_REJ"
	mkdir "$COMPAT_WIRELESS_REJ"
	find . -name "*.orig" | xargs -L 1 -I {} rm {} -f
	find . -name "*.rej" | cpio -pvdmu "$COMPAT_WIRELESS_REJ/"
	find . -name "*.rej" | grep -v "$COMPAT_WIRELESS_REJ/" | xargs -L 1 -I {} rm {} -f

	#copy admin update result
	rm -fR "$ADMIN_UP_RES_DIR"
	mkdir "$ADMIN_UP_RES_DIR"
	cp -R net "$ADMIN_UP_RES_DIR/"
	cp -R drivers "$ADMIN_UP_RES_DIR/"
	cp -R include "$ADMIN_UP_RES_DIR/"

	#apply TI patches
	patch -p 0 < "$TI_COMPAT_PATCH"
	if [ "$?" -ne 0 ]; then		
		#move rejects out of the way
		mkdir "$TI_REJ"
		find . -name "*.rej" | grep -v "$COMPAT_WIRELESS_REJ/" | cpio -pvdmu "$TI_REJ/"
		find . -name "*.rej" | grep -v "$COMPAT_WIRELESS_REJ/" | grep -v "$TI_REJ" | xargs -L 1 -I {} rm {} -f
		echo_red "Failed to apply TI compat patches, correct the sources and restart the script with $CREATE_PATCH_PARAM parameter.";
		exit 1;
	fi


}


function main() {

    if [ "$1" == "help" ]; then
        echo "usage tibluez.sh [ $CREATE_PATCH_PARAM | $MAKE_ONLY_PARAM ]"
        echo "When without parameters, will refresh and make (no patch creation)"
        exit 0
    fi
	#BEGIN
	echo_bold "Begin"

	#if final parameter is given, jump to make phase
	if [ "$1" !=  "$CREATE_PATCH_PARAM" -a "$1" != "$MAKE_ONLY_PARAM" ]; then
		praparation_phase
	fi

	if [ "$1" ==  "$GATHER_ONLY_PARAM" ]; then
		exit 0
	fi

	echo "Running make phase..."

	#select build
	./scripts/driver-select bt
	#run make ( P1 stage )
	make ARCH=arm KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD -j$THRD_COUNT
	if [ "$?" -ne 0 ]; then
		echo "Build failed! Correct the sources and restart the script with $CREATE_PATCH_PARAM parameter";
		exit 1;
	else
		rm -fr kos
		mkdir -p kos
		KOS=$(find . -name "*.ko")
		for ii in $KOS
		do
			echo "cp $ii kos/"
			cp $ii kos/
		done
        if [ "$1" ==  "$CREATE_PATCH_PARAM" ]; then
		    #clean to make a patch
		    make ARCH=arm KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD clean;
		    find . -name "*.order" | xargs -L 1 -I {} rm {} -f
		    find . -name "*.orig" | xargs -L 1 -I {} rm {} -f
		    #overwrite last good build
		    rm -fR "$LAST_GOOD_DIR";
		    cp -R net "$LAST_GOOD_DIR/";
		    cp -R drivers "$LAST_GOOD_DIR/";
		    cp -R include "$LAST_GOOD_DIR/";
		    #create new patches
		    diff -ruN "$ADMIN_UP_RES_DIR/net" net > "$TI_COMPAT_PATCH";
		    diff -ruN "$ADMIN_UP_RES_DIR/include" include >> "$TI_COMPAT_PATCH";
		    diff -ruN "$ADMIN_UP_RES_DIR/drivers" drivers >> "$TI_COMPAT_PATCH";

		    #back up the good patch
		    COMP_WIRE=`git show | grep commit | awk '{ print $2 }'`
		    pushd .
		    cd $GIT_COMPAT_TREE
		    COMP=`git show | grep commit | awk '{ print $2 }'`
		    popd
		    pushd .
		    cd $GIT_TREE
		    BLUE_NEXT=`git show | grep commit | awk '{ print $2 }'`
		    popd
		    COMB=$TI_COMPAT_PATCH-$COMP_WIRE-$COMP-$BLUE_NEXT
		    cp "$TI_COMPAT_PATCH" "$COMB"

		    #at last make build
		    make ARCH=arm KLIB=$KLIB KLIB_BUILD=$KLIB_BUILD -j$THRD_COUNT
	    fi
	fi

	echo "Done."
}


main $*
