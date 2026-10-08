#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
bin_dir="$HOME/.local/bin"
applications="$HOME/.local/share/applications"
profile="${DSH_APP_PROFILE:-$HOME/.local/share/dsh-app-firefox}"

link_plugin() {
	local node_modules="${DSH_HOME:-$HOME/.dsh}/profiles/web/node_modules"
	if [ -L "$node_modules/taskboard-deeplink" ] || [ -e "$node_modules/taskboard-deeplink" ]; then
		ln -sfn "$here" "$node_modules/taskboard-deeplink"
	else
		dsh plugin --profile web add "$here"
	fi
}

install_launcher() {
	mkdir -p "$bin_dir" "$applications"
	ln -sfn "$here/dsh-app" "$bin_dir/dsh-app"
	sed "s|^Exec=dsh-app|Exec=$bin_dir/dsh-app|" "$here/dsh-app.desktop" > "$applications/dsh-app.desktop"
	update-desktop-database "$applications" 2>/dev/null || true
	xdg-mime default dsh-app.desktop x-scheme-handler/dsh-app
}

drop_obsolete_app_prefs() {
	local prefs="$profile/user.js"
	[ -f "$prefs" ] || return 0
	sed -i '/dom\.allow_scripts_to_close_windows\|dom\.disable_window_flip/d' "$prefs"
}

link_plugin
install_launcher
drop_obsolete_app_prefs
echo "taskboard-deeplink installed. Restart dsh web (systemctl --user restart dsh-web) and the DSH app window."
