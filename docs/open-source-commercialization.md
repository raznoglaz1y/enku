# ENKU open-source + commercialization strategy

Status: **project policy baseline / October 2026**

ENKU may remain fully open-source hardware/software **and** be sold commercially through the project website or a crowdfunding campaign.

These are compatible goals.

## Why open source and Kickstarter are compatible

Kickstarter does not require a hardware project to be closed-source. Its current rules focus on:
- creating something new;
- truthful and clear presentation;
- showing the real state of a hardware/software prototype;
- not presenting unimplemented features as though they already work.

Open-source hardware projects have repeatedly used Kickstarter successfully. Examples include MAKERbuino, OpenBCI and more recent open-source hardware campaigns.

References:
- Kickstarter Creator Handbook: https://www.kickstarter.com/help/handbook
- Kickstarter Rules: https://www.kickstarter.com/rules
- MAKERbuino Kickstarter: https://www.kickstarter.com/projects/albertgajsak/makerbuino-a-diy-game-console
- OpenBCI Kickstarter: https://www.kickstarter.com/projects/openbci/openbci-an-open-source-brain-computer-interface-fo/

## Open-source does not mean "must be sold at cost"

Open-source hardware means that downstream users are allowed to study, modify, make, distribute and sell hardware based on the released design under the applicable license.

It does **not** require the original project to sell hardware at BOM cost.

Commercial price may legitimately cover:
- design and engineering;
- prototype failures;
- manufacturing setup;
- assembly;
- testing and QA;
- certification / compliance;
- packaging;
- warehousing;
- payment fees;
- customer support;
- returns and warranty;
- project development;
- future firmware work;
- taxes;
- operating margin.

OSHWA explicitly recognizes commercial open-source hardware and DIY kits as a commercialization path.

References:
- https://oshwa.org/resources/open-source-hardware-faq/
- https://oshwa.org/resources/open-source-hardware-definition/
- https://certification.oshwa.org/process/hardware.html

## ENKU licensing policy

Current intended structure:

### Hardware and mechanical source
CERN-OHL-P-2.0

This is a permissive open-hardware license.

It allows downstream users to build and commercialize derivatives without requiring all derivative hardware to remain under the same license.

### Firmware / software
Apache-2.0

Permissive software license with explicit patent provisions.

### Documentation
CC BY 4.0

Allows reuse, including commercial reuse, with attribution.

This licensing mix remains compatible with commercial kit sales.

## Trademark strategy

Open design does **not** mean that third parties automatically gain rights to the ENKU name/logo.

The ENKU name, logo and product identity should be treated separately from the open design files.

Recommended project policy:
- anyone may make and sell compatible hardware as allowed by the source licenses;
- third-party products should not imply that they are official ENKU products;
- derivatives should use a different product name unless explicit trademark permission is granted;
- official project hardware may be labeled "ENKU Original" / "Official ENKU Kit" later if useful.

This aligns with OSHWA guidance that derivatives must not imply that the original designer manufactured, sold, warranted or sanctioned them.

## Source-release timing

Two valid approaches exist.

### Open during development

Advantages:
- strong community credibility;
- contributors can review hardware early;
- creates public prior art;
- fits the current ENKU GitHub workflow.

Risk:
- copycats can follow development before launch.

### Publish production sources at campaign / shipment milestone

OSHWA notes that companies sometimes publish design files when the product goes on sale or after product release. A product should not be called fully open-source hardware until the source files are actually available.

For ENKU, the preferred model is:

- keep architecture, firmware and design development public;
- do not publish unsafe / unverified "production" Gerbers as fabrication-ready;
- publish the validated schematic, PCB source, BOM and enclosure files with the first production-ready hardware revision;
- clearly label WIP files as WIP until validation is complete.

This protects users from fabricating known-incomplete revisions without turning ENKU into a closed project.

## Margin policy

Open-source pricing should be **fair**, not zero-margin.

The project should use the same quality targets whether units are:
- self-built from public sources;
- sold as website kits;
- sold through Kickstarter;
- sold later as assembled devices.

Commercial pricing should use a full COGS model, not raw component BOM.

Recommended layers:

```text
raw components
+ PCB / assembly
+ display
+ battery
+ enclosure
+ packaging
+ QA / test allowance
+ yield / rework reserve
+ inbound freight
= landed manufacturing cost

+ payment / platform fees
+ VAT / taxes
+ fulfillment reserve
+ warranty / returns reserve
+ development contribution
+ operating margin
= selling price
```

## Target gross contribution philosophy

For direct website / crowdfunding planning:

- Electronics Kit: lower margin is acceptable because it grows the maker community.
- Base: protect a healthy margin; it is likely the volume product.
- Cover: should be margin-accretive.
- Pro: must fund the extra optical/mechanical complexity.
- Pro Wireless: must not be priced as a token +20 PLN feature; Qi adds validation, thickness and support risk.
- Dock: accessory should carry healthy contribution because its BOM is low but mechanical/fulfillment work is real.

No variant should be launched only because raw BOM leaves a small positive difference to sale price.

## Cost-down rule

ENKU should optimize price **without sacrificing critical quality**.

Never cost-cut:
- battery safety / charge-path protection;
- USB-C mechanical reliability;
- e-paper HV safety margins;
- tactile reliability of primary page controls;
- display/frontlight optical quality;
- reverse-current protection between USB / Dock / Qi;
- RF keepout;
- ESD protection where required.

Prefer cost-down through:
- distributor/source selection;
- common footprints and population options;
- lower-cost but qualified connectors;
- integrated power ICs where they remove passives / test risk;
- DNP features for Base;
- common PCB across variants;
- volume quotes;
- simpler enclosure assembly;
- reducing SKU count.

## Kickstarter policy

Do not launch ENKU on Kickstarter until there is a working physical prototype demonstrating the core promise.

Minimum campaign-ready prototype:
- real target display;
- real physical controls;
- working reader flow;
- microSD;
- USB-C charging;
- representative enclosure;
- working sleep/power behavior;
- at least one functioning Dock Mode flow if Dock is marketed;
- real frontlight prototype if Pro is marketed;
- real Qi prototype if Pro Wireless is marketed.

Any feature not yet implemented must be labeled as planned / stretch / development work rather than demonstrated functionality.

This follows Kickstarter's hardware transparency rules.

## Recommended Kickstarter structure

Main reward ladder:
- Electronics Kit;
- Base;
- Cover;
- Pro;
- Pro Wireless.

Add-ons:
- Dock;
- extra Cover;
- replacement enclosure parts;
- optional battery only if shipping rules and compliance permit.

Avoid too many launch SKUs. If manufacturing complexity becomes excessive, Pro Wireless may be a post-campaign variant or limited reward.

## Strategic rule

Open source is part of ENKU's product value.

It should not be treated as the reason margins must be small.

The economic goal is:

> make self-building possible, while making the official kit sufficiently convenient, tested and fairly priced that supporting the original project is the easiest choice.
