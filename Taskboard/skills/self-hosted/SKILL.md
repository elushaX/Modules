---
name: self-hosted
description: >-
  Topology, service map, storage layout, and operations for the user's self-hosted
  Arch Linux home server (ilusha-basic.online). Use whenever working on that server:
  managing/adding/debugging Docker services (Immich, Jellyfin, Navidrome, Feishin,
  Matrix, Mattermost, Jitsi, Stoat, Forgejo, Vaultwarden, Plane, etc.), the Caddy
  reverse proxy, the btrfs RAID storage, backups, or anything under ilusha-basic.online.
  Also covers the separate living-room HTPC / Steam Big Picture box on the LAN
  (ssh tv@192.168.1.212) and its Bluetooth game-controller setup.
---

# Self-hosted home server

Single Arch Linux box reachable at `ilusha-basic.online`, running ~20 Docker Compose
stacks fronted by a host-level Caddy reverse proxy, with a btrfs RAID1 storage pool.

> All facts below were captured from the live server on 2026-08-24 and confirmed by the
> user. Verify anything before destructive actions — the server changes over time.

## Access

- **SSH:** `ssh -p 2233 auser@ilusha-basic.online` (user `auser`).
- **sudo needs a password** — it is NOT passwordless. Read-only inspection runs fine as
  `auser`; anything requiring root (systemd units in `/etc`, btrfs scrub, mounts) must be
  run by the user. Stage a script in `~` and have them run it via their own terminal:
  `! ssh -t -p 2233 auser@ilusha-basic.online 'sudo bash ~/script.sh'`
- **OS:** Arch Linux, hostname `server`, single physical desktop box.

## Inbound / networking

- **Base domain:** `ilusha-basic.online`; every service is a `*.ilusha-basic.online` subdomain.
- **DNS provider:** reg.ru (account panel: https://www.reg.ru/user/account/card/115706357/nss/).
- **Inbound path:** ISP **public IP + port forwarding** on the home router to this box.
  (See also the `home-server-network` memory for router/WAN details.)
- Some containers proxy **outbound** traffic through the `sing-box` VPN container at
  `http://sing-box:7890` (e.g. Jellyfin sets HTTP(S)_PROXY to it).

## Reverse proxy (Caddy on the host, not in Docker)

- Service: `systemctl status caddy`. Config: `/etc/caddy/Caddyfile` + `/etc/caddy/conf.d/*`.
- Automatic HTTPS. Admin API is on a unix socket (`/run/caddy/admin.socket`).
- Pattern: each service = one site block reverse-proxying `sub.ilusha-basic.online`
  to a **localhost port**. Most app containers bind only to `127.0.0.1:<port>`; Caddy is
  the only public entry.
- Reload after edits: `sudo systemctl reload caddy` (or `caddy reload`).

### DNS / port map (subdomain → localhost port → service)

| Subdomain            | Port                 | Service                                             |
|----------------------|----------------------|-----------------------------------------------------|
| photos               | 2283                 | Immich (photos)                                     |
| movie                | 8096                 | Jellyfin (video)                                    |
| navidrome            | 4533                 | Navidrome (music server)                            |
| music                | 9180                 | Feishin (Navidrome web client)                      |
| beets                | 5001                 | beets — **staged but currently unused**             |
| matrix               | 8009 / 8083 / 7880 / 8010 | Tuwunel (Matrix HS) + lk-jwt + LiveKit + Cinny |
| chat                 | 8880                 | Stoat (Revolt-family chat)                          |
| mm                   | 8065                 | Mattermost                                          |
| jitsi                | 8000                 | Jitsi Meet                                          |
| room                 | 3030                 | hommyroom                                           |
| zulip                | 180                  | Zulip                                               |
| cloud                | 11000                | Nextcloud                                           |
| note                 | 3456                 | Vikunja (tasks/notes)                               |
| planning             | 3457                 | Plane (project planning)                            |
| dashboard            | 3001                 | Grafana (monitoring stack)                          |
| git                  | 880                  | Forgejo                                             |
| vault                | 8222                 | Vaultwarden (Bitwarden)                             |
| files                | 8055                 | Filebrowser                                         |
| psearch              | 9696                 | Prowlarr (in `/etc/caddy/conf.d/psearch.conf`)      |
| pirates              | 8090                 | qBittorrent                                         |
| smp                  | 5555                 | SimpleX SMP server                                  |

Also running without a dedicated subdomain: FlareSolverr, AudioMuse-AI, SimpleX XFTP,
`sing-box` (outbound VPN proxy `:7890`), `autoheal`, and the monitoring stack
(prometheus, cadvisor, node-exporter, grafana-image-renderer).

The `matrix.ilusha-basic.online` block is special: it serves `.well-known/matrix/*`
delegation, routes `/_matrix|/_synapse` → Tuwunel (8009), `/livekit-jwt/*` → lk-jwt (8083),
`/livekit/*` → LiveKit SFU (7880), and everything else → Cinny web client (8010).

## Service layout convention

- One directory per stack under `/home/auser/<service>/`, each with its own
  `docker-compose.yml` (`docker compose ls` lists all ~20 projects).
- App **config/state/DBs** are bind-mounted to `/mnt/hhd/metadata/<service>/`.
- **User content** (media, uploads) lives under `/mnt/hhd/data/...`.
- Containers publish to `127.0.0.1:<port>`; Caddy fronts them with TLS.

### Adding a new service

1. `mkdir /home/auser/<service>` and add a `docker-compose.yml`.
2. Bind app data to `/mnt/hhd/metadata/<service>/`; publish `127.0.0.1:<port>:<port>`.
   Route outbound through `sing-box:7890` only if the service needs the VPN.
3. `cd /home/auser/<service> && docker compose up -d`.
4. Add a Caddy block (in `Caddyfile` or a `conf.d/*.conf` file):
   `sub.ilusha-basic.online { reverse_proxy localhost:<port> }`, then
   `sudo systemctl reload caddy`.
5. Create the `sub` DNS record at reg.ru; the router already forwards 80/443 to this box.

## Storage architecture

| Mount        | Device(s)                         | FS            | Size / use                                   |
|--------------|-----------------------------------|---------------|----------------------------------------------|
| `/`          | `nvme0n1p2` (AirDisk 512G)         | ext4          | 231G — OS/root. `/boot` on p1, swap on p3.   |
| `/mnt/hhd`   | `sda` + `sdb` (2× 1TB WD HDD)      | **btrfs RAID1** | label `data`; ~384G used / ~548G free. Primary redundant store. |
| `/mnt/ssd`   | `sdc1` (1TB ADATA SATA SSD)        | ext4          | **~91% full (809G/938G)**; holds bulk media. NOT redundant. |
| (unmounted)  | `nvme1n1` (1TB ADATA 700)          | ext4/ntfs     | a separate Linux install / spare, not used by the server. |

- **btrfs RAID1** = both HDDs mirrored (data + metadata RAID1). Losing one disk keeps data.
- ⚠️ `/mnt/ssd` is a **single ext4 SSD, ~91% full, no redundancy** — the largest media
  (`/mnt/ssd/data/streaming/movies`) lives here. Watch free space; a failure loses it.

### Data organization (on `/mnt/hhd`)

- `metadata/<service>/` — per-service app data & databases (immich, jellyfin, navidrome,
  matrix, vaultwarden, mattermost, monitoring, stoat, zulip, ...).
- `data/` — user content: `streaming/` (`music`, `podcasts`, and `movies` → **symlink** to
  `/mnt/ssd/data/streaming/movies`), `personal/` (Immich uploads at `personal/images`),
  `documents/`, `archive/`, `backups/`, `configs/`, `repositories/`.
- `backups/immich/`, `incomplete/` (in-progress torrent downloads), `other/` (old backups).

### Media paths

- **Jellyfin:** `/mnt/hhd/data/streaming/movies` (→ SSD) and `/mnt/hhd/data/streaming/podcasts`.
- **Navidrome:** music at `/mnt/hhd/data/streaming/music` (on the HDD RAID1, mounted `:ro`).
- **Immich:** uploads `UPLOAD_LOCATION=/mnt/hhd/data/personal/images`, DB in `metadata/immich`.
- **qBittorrent:** downloads into `/mnt/hhd` (`/downloads`), also sees `/mnt/ssd`.

## Backups

- **Navidrome:** backed up via git revision (config/state tracked).
- **Immich:** **NOT backed up automatically** (a `/mnt/hhd/backups/immich` dir exists but is
  not on a schedule) — flag this if the user is relying on it.

## btrfs RAID auto-healing (scrub)

btrfs scrub reads every block, verifies checksums, and **repairs corruption from the good
RAID1 mirror** — the "self-heal" for the array. Managed by the btrfs-progs systemd template
`btrfs-scrub@mnt-hhd.timer` (instance name = `systemd-escape -p /mnt/hhd`).

- Schedule override lives in
  `/etc/systemd/system/btrfs-scrub@mnt-hhd.timer.d/override.conf`.
- Configured to run **every 3 days at 03:00** (`OnCalendar=*-*-01/3 03:00:00`), replacing the
  btrfs-progs default (monthly) and the earlier `weekly` setting.
- Install/adjust via the staged script: `~/raid-scrub-every3days.sh` on the server, run as
  root: `sudo bash ~/raid-scrub-every3days.sh`.
- Check status: `systemctl list-timers btrfs-scrub@mnt-hhd.timer`,
  logs `journalctl -u btrfs-scrub@mnt-hhd.service`, manual run `sudo btrfs scrub start -B /mnt/hhd`,
  health `sudo btrfs device stats /mnt/hhd` and `btrfs filesystem usage /mnt/hhd`.

## Other operational notes

- The Claude Code remote-control agent runs as a **systemd user** service `claude-rc.service`
  (`~/claude-agent/rc.log`); its OAuth access token is short-lived — restart the service to
  force a token refresh if it appears "logged out".
- Many stacks include their own Postgres/Redis/MinIO/RabbitMQ containers (plane, stoat,
  mattermost, immich, audiomuse) — don't confuse them with a shared DB; each is per-stack.

## Related machine: living-room HTPC (Steam Big Picture)

A **separate** box from the server above — do not confuse the two. This is the
"big picture arch linux machine" used for gaming on the TV.

- **SSH:** `ssh tv@192.168.1.212` (user `tv`, LAN-only; not exposed via the server's
  port forwarding). Hostname `archlinux`, Intel-based laptop/NUC used as an HTPC.
- **Purpose:** runs **Steam Big Picture** on the TV; controllers connect over Bluetooth.
- **Bluetooth stack:** `bluez` + `bluez-utils` installed, `bluetooth.service` active +
  enabled. Adapter `hci0` (not rfkill-blocked). `bluetoothctl` is available here.
- **Paired devices** (as of 2026-08-26):
  - `A0:5A:58:3F:56:41` — **Sony DualShock 4 v2** (`054C:09CC`, Modalias
    `usb:v054Cp09CCd0001`), advertised as "Wireless Controller". Over USB it enumerates as
    "Sony Interactive Entertainment Wireless Controller" (`hid-playstation`) with `js0` +
    Motion Sensors + Touchpad nodes; over BT it appears as just "Wireless Controller".
  - `20:18:5B:46:EF:13` — JBL Go 4 Bluetooth speaker.
- **Controller-over-Bluetooth troubleshooting** (works wired, dead wireless):
  - Wired uses the kernel HID driver directly, so it always works.
  - **Known root cause here (2026-08-26):** `bluetoothctl info <MAC>` showed
    `Trusted: yes` but `Paired: no` / `Bonded: no` / `CablePairing: yes` — the device
    record was auto-created from the USB cable but **never actually BT-paired** (no link
    key), so it associates over the air ("shows connected") but exposes no `js0` and no
    input. `trust` alone does NOT create a pairing. Fix = a real BT pair: `bluetoothctl
    remove <MAC>`, put the DS4 in pairing mode (hold SHARE + PS ~5s until the light bar
    double-flashes), then `scan on` → `pair` → `trust` → `connect`. Staged as
    `~/re-pair-ds4.sh` on the box. After pairing, `Paired`/`Bonded` should read `yes`.
  - Confirm the input node actually appears when connected wirelessly:
    `ls /dev/input/js*` and watch `journalctl -k -f` while reconnecting; test with
    `evtest /dev/input/event15` (or the current node) to see button events.
  - **Xbox pads only:** if it drops instantly, disable ERTM —
    `echo 'options bluetooth disable_ertm=1' | sudo tee /etc/modprobe.d/bluetooth.conf`
    then reboot. `disable_ertm` is `N` (enabled) by default here; PlayStation pads don't
    need this.
  - In Steam, enable the matching option under **Settings → Controller**
    (PlayStation/Xbox/Switch support) if Steam ignores an otherwise-connected pad.

## Forgejo (git.ilusha-basic.online) — git hosting

- Web/API: `https://git.ilusha-basic.online` (Caddy → localhost:880). Forgejo 11.0.15.
- Username: `ilusha`. Git SSH clone via the host sshd on **port 2233** (user `git`):
  `git@git.ilusha-basic.online:2233` → remote URL
  `ssh://git@git.ilusha-basic.online:2233/ilusha/<repo>.git`.
- API token (saved 2026-09-25): `<get_secret("selfhost.forgejo_token")>`.
  ⚠️ This token is **scope-limited** — it lacks `read:user`/`write:user`/`write:repository`,
  so it CANNOT create repos or read the user endpoint over the API. It's only good for
  read-only calls that need no user scope. To create a repo, either generate a token with
  `write:repository` (Settings → Applications), make the empty repo in the web UI, or use
  the server CLI: `ssh -p 2233 auser@ilusha-basic.online` then
  `docker exec -u git <forgejo-container> forgejo admin ...` (needs sudo/docker access).
