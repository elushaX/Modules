import { adapters, loadAccounts, updateAccount } from "@subrouter/cli";

const provider = "anthropic";
const file = await loadAccounts();
const pool = file.providers?.[provider];
const account = pool?.accounts?.[pool.activeIndex ?? 0];
if (!account) {
	process.stderr.write("subrouter has no anthropic account; run: npx @subrouter/cli login anthropic\n");
	process.exit(1);
}

const match = { ...account };
const persist = (update) => updateAccount({ provider, match, update });
const token = await adapters.anthropic.getApiKey({ account, persist });
if (token instanceof Error) {
	process.stderr.write(`${token.message}\n`);
	process.exit(1);
}
process.stdout.write(token);
