package online.ilushabasic.sketch2d;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.os.Build;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;

public final class Updater {
	private static final int kTimeoutMs = 8000;
	private static final int kMaximumManifestBytes = 256 * 1024;
	private static final String kInstallAction = "online.ilushabasic.sketch2d.INSTALL_STATUS";

	private Updater() {}

	public static long installedVersionCode(Activity activity) {
		try {
			PackageInfo info = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
			return info.getLongVersionCode();
		} catch (PackageManager.NameNotFoundException error) {
			return 0;
		}
	}

	public static long installedUpdateTime(Activity activity) {
		try {
			PackageInfo info = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
			return info.lastUpdateTime / 1000;
		} catch (PackageManager.NameNotFoundException error) {
			return 0;
		}
	}

	public static String fetchText(String address) {
		try {
			HttpURLConnection connection = open(address);
			try (InputStream input = connection.getInputStream()) {
				ByteArrayOutputStream output = new ByteArrayOutputStream();
				byte[] buffer = new byte[8192];
				int read;
				while ((read = input.read(buffer)) > 0) {
					output.write(buffer, 0, read);
					if (output.size() > kMaximumManifestBytes) return null;
				}
				return output.toString("UTF-8");
			} finally {
				connection.disconnect();
			}
		} catch (Exception error) {
			return null;
		}
	}

	public static String download(Activity activity, String address, String expectedSha256, long handle) {
		File target = new File(activity.getCacheDir(), "update.apk");
		try {
			HttpURLConnection connection = open(address);
			long total = connection.getContentLengthLong();
			MessageDigest digest = MessageDigest.getInstance("SHA-256");
			try (InputStream input = connection.getInputStream(); OutputStream output = new FileOutputStream(target)) {
				byte[] buffer = new byte[64 * 1024];
				long received = 0;
				int read;
				while ((read = input.read(buffer)) > 0) {
					output.write(buffer, 0, read);
					digest.update(buffer, 0, read);
					received += read;
					onProgress(handle, received, total);
				}
			} finally {
				connection.disconnect();
			}
			if (!hex(digest.digest()).equalsIgnoreCase(expectedSha256)) {
				target.delete();
				return "The download was damaged. Try again later.";
			}
			return null;
		} catch (Exception error) {
			target.delete();
			return "Could not download the update: " + error.getMessage();
		}
	}

	public static String install(Activity activity) {
		File apk = new File(activity.getCacheDir(), "update.apk");
		try {
			PackageInstaller installer = activity.getPackageManager().getPackageInstaller();
			PackageInstaller.SessionParams params =
				new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
			params.setAppPackageName(activity.getPackageName());
			int sessionId = installer.createSession(params);
			try (PackageInstaller.Session session = installer.openSession(sessionId)) {
				try (InputStream input = new FileInputStream(apk);
					 OutputStream output = session.openWrite("update.apk", 0, apk.length())) {
					byte[] buffer = new byte[64 * 1024];
					int read;
					while ((read = input.read(buffer)) > 0)
						output.write(buffer, 0, read);
					session.fsync(output);
				}
				registerConfirmation(activity);
				Intent intent = new Intent(kInstallAction).setPackage(activity.getPackageName());
				PendingIntent pending = PendingIntent.getBroadcast(
					activity, sessionId, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE
				);
				session.commit(pending.getIntentSender());
			}
			return null;
		} catch (Exception error) {
			return "Could not start the installer: " + error.getMessage();
		}
	}

	private static void registerConfirmation(Activity activity) {
		BroadcastReceiver receiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
				if (status != PackageInstaller.STATUS_PENDING_USER_ACTION) return;
				Intent confirm = confirmationIntent(intent);
				if (confirm == null) return;
				confirm.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				activity.startActivity(confirm);
			}
		};
		activity.getApplicationContext().registerReceiver(
			receiver, new IntentFilter(kInstallAction), Context.RECEIVER_NOT_EXPORTED
		);
	}

	@SuppressWarnings("deprecation")
	private static Intent confirmationIntent(Intent status) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
			return status.getParcelableExtra(Intent.EXTRA_INTENT, Intent.class);
		return status.getParcelableExtra(Intent.EXTRA_INTENT);
	}

	private static HttpURLConnection open(String address) throws Exception {
		HttpURLConnection connection = (HttpURLConnection) new URL(address).openConnection();
		connection.setConnectTimeout(kTimeoutMs);
		connection.setReadTimeout(kTimeoutMs);
		connection.setUseCaches(false);
		if (connection.getResponseCode() != HttpURLConnection.HTTP_OK)
			throw new Exception("HTTP " + connection.getResponseCode());
		return connection;
	}

	private static String hex(byte[] bytes) {
		StringBuilder text = new StringBuilder();
		for (byte value : bytes)
			text.append(String.format("%02x", value));
		return text.toString();
	}

	private static native void onProgress(long handle, long received, long total);
}
