#!/bin/bash
set -e
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/.." && pwd)
NOTE="$1"
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk ANDROID_HOME=$HOME/Android/Sdk
HOMESERVER=https://matrix.ilusha-basic.online
TOKEN=$(cat ~/.matrix-agent-token)
ROOM=$(python3 -c "import urllib.parse;print(urllib.parse.quote('!jemZbE9F2vPKp7T1L3:matrix.ilusha-basic.online'))")

cd "$REPO"
REVISION=$(git rev-parse --short HEAD)
(cd android && ./gradlew --no-daemon -q assembleRelease > /tmp/sketch2d-apk-release.log 2>&1)
APK=android/app/build/outputs/apk/release/app-release.apk
NAME="sketch2d-android-$REVISION.apk"
SIZE=$(stat -c %s "$APK")

UPLOAD=$(curl -s -m 600 -XPOST -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/vnd.android.package-archive" --data-binary @"$APK" "$HOMESERVER/_matrix/media/v3/upload?filename=$NAME")
MXC=$(echo "$UPLOAD" | python3 -c 'import sys,json;print(json.load(sys.stdin)["content_uri"])')

python3 -c "import json,sys;print(json.dumps({'msgtype':'m.file','body':sys.argv[1],'filename':sys.argv[1],'url':sys.argv[2],'info':{'mimetype':'application/vnd.android.package-archive','size':int(sys.argv[3])}}))" "$NAME" "$MXC" "$SIZE" > /tmp/sketch2d-apk-message.json
curl -s -m 60 -XPUT -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" -d @/tmp/sketch2d-apk-message.json "$HOMESERVER/_matrix/client/v3/rooms/$ROOM/send/m.room.message/apk$(date +%s%N)"
echo

python3 -c "import json,sys;print(json.dumps({'msgtype':'m.text','body':sys.argv[1]}))" "$NOTE" > /tmp/sketch2d-apk-note.json
curl -s -m 60 -XPUT -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" -d @/tmp/sketch2d-apk-note.json "$HOMESERVER/_matrix/client/v3/rooms/$ROOM/send/m.room.message/note$(date +%s%N)"
echo

MEDIA=${MXC#mxc://matrix.ilusha-basic.online/}
curl -s -m 600 -H "Authorization: Bearer $TOKEN" -o /tmp/sketch2d-apk-check.apk "$HOMESERVER/_matrix/client/v1/media/download/matrix.ilusha-basic.online/$MEDIA"
LOCAL=$(sha256sum "$APK" | cut -d' ' -f1)
REMOTE=$(sha256sum /tmp/sketch2d-apk-check.apk | cut -d' ' -f1)
rm -f /tmp/sketch2d-apk-check.apk
echo "sent $NAME local $LOCAL remote $REMOTE"
"$HERE/publish-sketch2d.sh" || echo "publish to ilusha-basic.online failed"
