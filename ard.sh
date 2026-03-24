#!/usr/bin/env bash

set -euo pipefail

FQBN="arduino:avr:uno"
BUILD_DIR="build"
SKETCH="ARGB.ino"
PORT=/dev/ttyUSB0

usage(){
	cat <<EOF
Usage: $0 <command> [arguments]

Commands:
	compile 		- Checks for compile-time errors
	build			- Builds sketch with .hex in $BUILD_DIR
	upload			- Build (if needed) and upload
	monitor			- Open port monitor
	write <text>	- Send <text> up USB with \n at the end

Examples:
	$0 compile
	$0 upload
	$0 monitor
	$0 write "hi"
EOF
	exit 1
}

# technically i'd need to validate input, but script within project's file
# and it will comple, build and upload from it
#check_sketch(){
#}

cmd_compile(){
	arduino-cli compile -b "$FQBN" --no-color "$SKETCH"
	echo "compiled"
}

cmd_build(){
	echo "building into .hex"
	arduino-cli compile -b "$FQBN" --warnings all --output-dir "$BUILD_DIR" "$SKETCH"
}

cmd_upload(){
	echo "bilding"
	arduino-cli compile -b "$FQBN" "$SKETCH"

	echo "uploading"
	arduino-cli upload -b "$FQBN" -p "$PORT" "$SKETCH"

	echo "finished"
}

cmd_monitor(){
	arduino-cli monitor -p $PORT --config 115200
}

cmd_write(){
	local text="$*"
	printf "%s" "$text" > $PORT
}

# main
if [[ $# -eq 0 ]]; then
	usage
fi

CMD="$1"
shift

case "$CMD" in
	compile) 	cmd_compile 	;;
	build) 		cmd_build 		;;
	upload) 	cmd_upload 		;;
	monitor) 	cmd_monitor 	;;
	write) 		cmd_write "$@" 	;;
	*) usage					;;
esac

