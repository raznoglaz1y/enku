# Third-party assets and release prerequisites

ENKU now has a defined licensing model for original project material. See [LICENSES.md](LICENSES.md).

- firmware/software: Apache-2.0;
- project documentation: CC BY 4.0;
- future project-owned hardware/mechanical source: CERN-OHL-P-2.0.

Branding, historical design boards and third-party assets remain separately scoped as described below.

## ENKU branding

The ENKU logo in `assets/branding/enku-logo.svg` is a project-owned asset supplied by the project creator.

The ENKU name and logo are branding assets and are not included in the blanket Apache-2.0 / CC BY 4.0 / CERN-OHL-P-2.0 grants unless explicitly stated otherwise.

## Fonts and icons

The UI uses **Noto Sans** as the current reading typography target and an approved set of **Phosphor** SVG icons in the design workflow.

Before public redistribution of bundled font/icon files, record:

- source;
- exact version;
- license;
- required attribution;
- whether redistribution is allowed.

Do not add font binaries to the repository unless redistribution rights have been verified.

## Historical design boards

Historical mockup boards may contain illustrative book covers, sample text or other reference material that requires review before a formal public release.

These boards are design references and should not be interpreted as redistributable content licenses for any third-party material visible inside them.

## Release prerequisites

Before the first tagged open-source release:

- verify third-party asset provenance;
- complete the design-asset licensing audit;
- add required attribution files;
- remove or replace any unlicensed illustrative content;
- document licenses for bundled dependencies/assets;
- add per-directory/per-file license notices where needed.
