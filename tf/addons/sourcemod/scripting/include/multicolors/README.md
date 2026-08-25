# multicolors (vendored)

Third-party colored-chat include set, vendored so this plugin builds from a clean checkout.

| | |
|---|---|
| Upstream | https://github.com/Bara/Multi-Colors |
| Version | 2.2.0 (`MuCo_VERSION`) |
| License | GNU GPL v3 - the same license this repository uses; see the root `LICENSE`. |
| Files | `../multicolors.inc`, `morecolors.inc`, `colors.inc` |

Credits carried from upstream: Popoklopsi, Powerlord, exvel, Dr. McKay.

## Why this is here

`parkourfortress.sp` needs `CPrintToChat` and `CReplyToCommand` for its colored chat
messages. It previously included `<morecolors>`, which was never vendored here and is not
part of a stock SourceMod install, so the plugin could not be compiled from a clean
checkout of this repository - including by its own `.github/workflows/build.yml`, which
downloads stock SourceMod and would fail on the same missing include.

Multi-Colors is the maintained successor to that include and provides the same
`CPrintToChat` / `CReplyToCommand` API.

Do not edit these files locally; re-vendor from upstream instead so the provenance above
stays accurate.
