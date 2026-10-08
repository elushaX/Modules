#!/bin/bash
set -euo pipefail

usage() {
	echo "usage: $0 start [avd] | gui [avd] | install | run | log | stop"
	echo "  start   cold-boot a clean headless emulator (default avd: sketch2d-api36) and wait until it is ready"
	echo "  gui     boot the emulator in a window you can click and type into, rendered on the host GPU;"
	echo "          its state is saved on stop and restored on the next start"
	echo "          (set SKETCH2D_EMULATOR_GPU=swiftshader_indirect to force software rendering)"
	echo "  install build the debug APK with Gradle and install it"
	echo "  run     install, start the app and print its log"
	echo "  log     follow the app log"
	echo "  stop    shut the emulator down"
	exit 1
}

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"
export JAVA_HOME="${JAVA_HOME_17:-/usr/lib/jvm/java-17-openjdk}"
ADB="$ANDROID_HOME/platform-tools/adb"
SERIAL="${ANDROID_SERIAL:-emulator-5554}"
PACKAGE="online.ilushabasic.sketch2d"
ACTIVITY="$PACKAGE/android.app.NativeActivity"

start() {
	local avd="${1:-sketch2d-api36}"
	local window="${2:-}"
	local port="${SERIAL#emulator-}"
	local display=(-no-window)
	local gpu="swiftshader_indirect"
	local boot=(-no-snapshot)
	if [ "$window" = "window" ]; then
		display=()
		gpu="host"
		boot=()
	fi
	gpu="${SKETCH2D_EMULATOR_GPU:-$gpu}"
	nohup "$ANDROID_HOME/emulator/emulator" -avd "$avd" "${display[@]}" "${boot[@]}" -no-audio -no-boot-anim \
		-gpu "$gpu" -port "$port" > "${TMPDIR:-/tmp}/sketch2d-emulator-$port.log" 2>&1 &
	"$ADB" -s "$SERIAL" wait-for-device
	until [ "$("$ADB" -s "$SERIAL" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; do sleep 2; done
	echo "$avd ready on $SERIAL"
}

install() {
	(cd "$ROOT/android" && ./gradlew --quiet assembleDebug)
	"$ADB" -s "$SERIAL" install -r "$ROOT/android/app/build/outputs/apk/debug/app-debug.apk"
}

run() {
	install
	"$ADB" -s "$SERIAL" shell am force-stop "$PACKAGE"
	"$ADB" -s "$SERIAL" logcat -c
	"$ADB" -s "$SERIAL" shell am start -W -n "$ACTIVITY"
	sleep 2
	"$ADB" -s "$SERIAL" logcat -d -s sketch2d:V
}

case "${1:-}" in
	start) start "${2:-}" ;;
	gui) start "${2:-}" window ;;
	install) install ;;
	run) run ;;
	log) "$ADB" -s "$SERIAL" logcat -s sketch2d:V ;;
	stop) "$ADB" -s "$SERIAL" emu kill ;;
	*) usage ;;
esac
