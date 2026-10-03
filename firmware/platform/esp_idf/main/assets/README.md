# Embedded Noto Sans asset

ENKU embeds `NotoSans-Regular.ttf` in the ESP-IDF application image so the
reader UI and pagination metrics do not depend on removable storage.

Source:

- repository: `notofonts/noto-fonts`
- path: `hinted/ttf/NotoSans/NotoSans-Regular.ttf`
- source blob SHA: `d55220958aa51eeeb85048d746eabe43d2cd9f14`
- ENKU blob SHA: `d55220958aa51eeeb85048d746eabe43d2cd9f14`
- license: SIL Open Font License 1.1

The adjacent `OFL-Noto.txt` file contains the license text distributed with
the source font.

At runtime an SD copy at `/system/fonts/NotoSans-Regular.ttf` is treated only
as an optional override. If it is absent or invalid, FreeType uses the embedded
firmware asset.
