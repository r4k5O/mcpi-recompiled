from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ANCHORS = ROOT / "docs" / "reverse-engineering" / "anchor-index.tsv"
CLASS_MAP = ROOT / "docs" / "reverse-engineering" / "class-map.md"

REQUIRED = {
    "NinecraftApp",
    "Minecraft",
    "Level",
    "LevelChunk",
    "ChunkSource",
    "ChunkCache",
    "RandomLevelSource",
    "Dimension",
    "LevelStorage",
    "ExternalFileLevelStorage",
    "Inventory",
    "Entity",
    "LevelRenderer",
    "EntityRenderer",
    "ClientSideNetworkHandler",
    "ServerSideNetworkHandler",
    "IPosTranslator",
    "OffsetPosTranslator",
}


def main() -> int:
    assert ANCHORS.exists(), "anchor-index.tsv must exist"
    assert CLASS_MAP.exists(), "class-map.md must exist"

    lines = ANCHORS.read_text(encoding="utf-8").splitlines()
    assert lines, "anchor index must not be empty"
    assert lines[0].split("\t") == [
        "subsystem",
        "symbol",
        "rtti_name_va",
        "typeinfo_va",
        "vtable_va",
        "first_virtual",
        "evidence",
        "confidence",
        "summary",
    ]

    seen = set()
    allowed_confidence = {"confirmed", "strong-inference", "open"}
    for line in lines[1:]:
        if not line.strip():
            continue
        fields = line.split("\t")
        assert len(fields) == 9, f"invalid anchor row: {line!r}"
        subsystem, symbol, rtti, typeinfo, vtable, first_virtual, evidence, confidence, summary = fields
        assert subsystem
        assert symbol
        assert confidence in allowed_confidence, (symbol, confidence)
        assert evidence
        assert summary
        for address in (rtti, typeinfo, vtable, first_virtual):
            if address:
                assert address.startswith("0x"), (symbol, address)
                int(address, 16)
        seen.add(symbol)

    missing = REQUIRED - seen
    assert not missing, f"missing required RE anchors: {sorted(missing)}"

    text = CLASS_MAP.read_text(encoding="utf-8")
    for required in (
        "Level -> ChunkSource -> RandomLevelSource -> LevelChunk",
        "levelchunk-layout.md",
        "api-coordinate-translation.md",
        "confirmed",
        "strong inference",
        "open",
    ):
        assert required in text, f"class map missing {required!r}"

    print(f"reverse-engineering anchor contract passed with {len(seen)} symbols")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
