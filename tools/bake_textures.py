"""Bakes Pocket Dungeon's textures into src/render/Textures.gen.h.

Each texture becomes one byte per pixel, an index into a small palette, and
LEVELS copies of that palette, each darker than the last (distance fog toward
black): the raycaster shades by picking a palette, as old consoles did. A
64x64 wall is 4 KB of indices and 512 bytes of palettes, where 8 full-colour
copies took 64 KB. Walls, floors and ceilings have exactly SURFACE_COLOURS
colours, so the raycaster can mix them along a row; a sprite's index 0 is
transparent (0xF81F in every palette), which the raycaster skips.

Run: python tools/bake_textures.py <art folder>
"""

import sys
from pathlib import Path

from PIL import Image

from fewer_colours import reduce_colours

LEVELS = 16
TRANSPARENT = 0xF81F
# The darkest level keeps this much of the colour, so far walls stay visible.
FLOOR_BRIGHTNESS = 0.10
# The hero's torch: near things lit warm, far things fading to a cool dark.
NEAR_TINT = (1.08, 0.96, 0.80)
FAR_TINT = (0.70, 0.82, 1.15)

# Textures drawn darker or brighter than the art: the ceiling sits back, the
# dark dragon would vanish in the fog.
SCALED = {"Ceiling": 0.8, "CeilingBone": 0.8, "CeilingWet": 0.8, "CeilingForge": 0.8, "InCeiling": 0.8}
# The other zones' surfaces are the first zone's, recoloured: each colour
# moved toward its brightness times the zone's colour, by `amount`, then
# scaled. Same art, so the same charm; a different mood.
RECOLOUR = {"Wet": ((0.80, 0.95, 1.20), 0.40, 0.82)}
SURFACE_NAMES = ("Wall", "Cracked", "Floor", "Ceiling")
# Each zone's light: (near, far) tints for textures whose name ends with it.
ZONE_TINTS = {
    "Bone": ((1.10, 0.98, 0.82), (0.80, 0.72, 0.62)),
    "Wet": ((0.95, 1.02, 1.10), (0.55, 0.78, 1.20)),
    "Forge": ((1.18, 0.95, 0.78), (1.10, 0.55, 0.45)),
}
# Sprites that are not standing on the floor: menu icons, things in the air,
# the hand in view, things seen from above (stacked slices). Every other
# sprite is moved down to stand on it.
FLOATING = ("Spell", "Bolt", "Torch", "Dust", "Hand")
# Textures that are never far away: one level, no fog.
UNLIT = {"Hand", "HandAxe", "HandSpear", "HandDagger", "HandHammer"}
# Sprites keep few colours, like old console art: this many for the 32 px
# ones, fewer for the small items. The hand in view keeps its own.
SPRITE_COLOURS = {32: 8, 16: 6}
# The shopkeeper keeps more, or his green goes brown.
COLOURS_BY_NAME = {"Merchant": 16}
# The village's people keep a few more than a monster, for their clothes.
COLOURS_BY_NAME.update({n: 12 for n in ("Granny", "Kid", "Fence", "KeeperSmith", "KeeperHerbs", "KeeperBar",
                                        "KeeperWizard", "KeeperBank")})

# name, file, size, is a sprite (has transparency). Floor looks must be the
# floor's size.
TEXTURES = [
    ("WallStone", "wall64_stone.png", 64, False),
    ("WallCracked", "wall64_cracked.png", 64, False),
    # Each zone's cracked wall after some blows, and after more
    # (make_walls.py).
    ("WallCracked1", "wall64_cracked_1.png", 64, False),
    ("WallCracked2", "wall64_cracked_2.png", 64, False),
    ("CrackedBone1", "wall64_bone_cracked_1.png", 64, False),
    ("CrackedBone2", "wall64_bone_cracked_2.png", 64, False),
    ("CrackedWet1", "wall64_wet_cracked_1.png", 64, False),
    ("CrackedWet2", "wall64_wet_cracked_2.png", 64, False),
    ("CrackedForge1", "wall64_forge_cracked_1.png", 64, False),
    ("CrackedForge2", "wall64_forge_cracked_2.png", 64, False),
    ("Door", "door64.png", 64, False),
    ("Floor", "floor32b.png", 32, False),
    ("FloorHole", "floor32_hole.png", 32, False),
    ("FloorSpikes", "floor32_spikes.png", 32, False),
    ("FloorRune", "floor32_rune.png", 32, False),
    ("Ceiling", "ceiling2.png", 32, False),
    # The other zones' surfaces, made from the first zone's (same style,
    # another material); the second wall is each zone's rarer one.
    ("WallBone", "wall64_bone.png", 64, False),
    ("WallBone2", "wall64_bone2.png", 64, False),
    ("CrackedBone", "wall64_bone_cracked.png", 64, False),
    ("FloorBone", "floor32_bone.png", 32, False),
    ("CeilingBone", "ceiling32_bone.png", 32, False),
    ("WallWet", "wall64_wet.png", 64, False),
    ("WallWet2", "wall64_wet2.png", 64, False),
    ("CrackedWet", "wall64_wet_cracked.png", 64, False),
    ("FloorWet", "floor32_wet.png", 32, False),
    ("CeilingWet", "ceiling32_wet.png", 32, False),
    ("WallForge", "wall64_forge.png", 64, False),
    ("WallForge2", "wall64_forge2.png", 64, False),
    ("CrackedForge", "wall64_forge_cracked.png", 64, False),
    ("FloorForge", "floor32_forge.png", 32, False),
    ("CeilingForge", "ceiling32_forge.png", 32, False),
    # Echo stones, one per zone: the zone's wall with a glowing tablet.
    ("WallEchoStone", "wall64_stone_echo.png", 64, False),
    ("WallEchoBone", "wall64_bone_echo.png", 64, False),
    ("WallEchoWet", "wall64_wet_echo.png", 64, False),
    ("WallEchoForge", "wall64_forge_echo.png", 64, False),
    # The village: house walls, cobbles, the ruin every building starts as,
    # and each building's front, in Village::Building order.
    ("VillageWall", "v_wall.png", 64, False),
    ("Cobble", "v_cobble.png", 32, False),
    ("Ruin", "v_ruin.png", 64, False),
    ("VBlacksmith", "v_blacksmith.png", 64, False),
    ("VApothecary", "v_apothecary.png", 64, False),
    ("VTavern", "v_tavern.png", 64, False),
    ("VTower", "v_tower.png", 64, False),
    ("VBank", "v_bank.png", 64, False),
    ("VLift", "v_lift.png", 64, False),
    ("Sky", "v_sky_seamless.png", 64, False),
    # The cloth roof over the alley (make_walls.py).
    ("Canopy", "v_canopy.png", 32, False),
    # The houses' upper storeys and roofs, standing on the walls: two for
    # any house, the tower's and the ruin's. Empty where the sky shows; filled
    # in down to the wall they stand on (make_walls.py).
    ("VTopA", "v_top_a_full.png", 64, True),
    ("VTopB", "v_top_b_full.png", 64, True),
    ("VTopTower", "v_top_tower_full.png", 64, True),
    ("VTopRuin", "v_top_ruin_full.png", 64, True),
    # Inside the buildings: plank walls, floorboards, a beamed ceiling, and
    # each building's keeper, in Village::Building order (the lift has none).
    ("InWall", "in_wall.png", 64, False),
    ("InFloor", "in_floor.png", 32, False),
    ("InCeiling", "in_ceiling.png", 32, False),
    ("KeeperSmith", "k_smith.png", 32, True),
    ("KeeperHerbs", "k_herbs.png", 32, True),
    ("KeeperBar", "k_bar.png", 32, True),
    ("KeeperWizard", "k_wizard.png", 32, True),
    ("KeeperBank", "k_bank.png", 32, True),
    # Monsters, in MonsterKind order.
    ("Droolblob", "m_drool.png", 32, True),
    ("Candleghoul", "r_candle.png", 32, True),
    ("Eyejar", "g_jar.png", 32, True),
    ("Eyebat", "d_eyebat.png", 32, True),
    ("Flamespider", "m_lantern.png", 32, True),
    ("Bonehound", "m_pup.png", 32, True),
    ("Teasnail", "g_slug.png", 32, True),
    ("Stewpot", "n_pot.png", 32, True),
    ("Grimoire", "d_book.png", 32, True),
    ("Ragmummy", "m_mummy.png", 32, True),
    ("Slimewizard", "sw3.png", 32, True),
    ("Rockstack", "d_rocks2.png", 32, True),
    ("Glimmerjelly", "glimmerjelly.png", 32, True),
    ("Mimic", "mimic.png", 32, True),
    ("Grubking", "n_grub.png", 32, True),
    ("Hydragon", "g_dragon2.png", 32, True),
    ("Keeper", "keeper2.png", 32, True),
    ("Ghost", "ghost.png", 32, True),
    # Items, in ItemKind order.
    ("Potion", "potion.png", 16, True),
    ("Gold", "gold2.png", 16, True),
    ("Sword", "icon_whetstone.png", 16, True),
    ("Shield", "shield.png", 16, True),
    ("Heart", "heart.png", 16, True),
    ("Chest", "chest.png", 32, True),
    ("Key", "key2.png", 16, True),
    ("Scroll", "scroll3.png", 16, True),
    ("Bomb", "bomb.png", 16, True),
    ("Wing", "wing.png", 16, True),
    ("Elixir", "elixir.png", 16, True),
    ("Page", "item_page.png", 32, True),
    ("Blade", "sword.png", 16, True),
    ("Axe", "icon_axe.png", 16, True),
    ("Spear", "icon_spear.png", 16, True),
    ("Dagger", "icon_dagger.png", 16, True),
    ("Hammer", "icon_hammer.png", 16, True),
    ("Shard", "shard2.png", 16, True),
    ("Duck", "duck.png", 16, True),
    ("Snack", "snack2.png", 16, True),
    ("Lockpick", "lockpick.png", 16, True),
    ("Gem", "gem.png", 16, True),
    ("Goblet", "goblet.png", 16, True),
    ("Note", "note.png", 16, True),
    ("Moss", "mat_moss.png", 16, True),
    ("Bone", "mat_bone.png", 16, True),
    ("Shell", "mat_shell.png", 16, True),
    ("Ore", "mat_ore.png", 16, True),
    ("Water", "item_water.png", 16, True),
    ("Glowcap", "item_glowcap.png", 16, True),
    ("Fish", "item_fish.png", 16, True),
    ("Pearl", "item_pearl.png", 16, True),
    ("Vial", "item_vial.png", 16, True),
    # Spells: their icons, and the fire bolt in flight.
    ("SpellFire", "spell_fire.png", 16, True),
    ("SpellHeal", "spell_heal.png", 16, True),
    ("SpellFrost", "spell_frost.png", 16, True),
    ("SpellBlink", "spell_blink.png", 16, True),
    ("SpellFold", "spell_fold.png", 16, True),
    ("Bolt", "fireball.png", 16, True),
    # Features, in FeatureKind order.
    ("Fountain", "fountain.png", 32, True),
    ("Merchant", "r_frog2.png", 32, True),
    ("Shrine", "shrine.png", 32, True),
    ("PortalHome", "portal_home.png", 32, True),
    ("Granny", "v_granny.png", 32, True),
    ("Kid", "v_kid.png", 32, True),
    ("Fence", "m_bandit.png", 32, True),
    ("Board", "prop_board.png", 32, True),
    # Zone props, kZoneProps per zone in zone order.
    ("PropPack", "prop_pack.png", 32, True),
    ("PropBarrel", "prop_barrel.png", 32, True),
    ("PropBush", "prop_bush.png", 32, True),
    ("PropSkulls", "prop_skulls.png", 32, True),
    ("PropCandles", "prop_candles.png", 32, True),
    ("PropCoffin", "prop_coffin.png", 32, True),
    ("PropCrystal", "prop_crystal.png", 32, True),
    ("PropMushrooms", "prop_mushrooms.png", 32, True),
    ("PropClam", "prop_clam.png", 32, True),
    ("PropAnvil", "prop_anvil.png", 32, True),
    ("PropBrazier", "prop_brazier.png", 32, True),
    ("PropRack", "prop_rack.png", 32, True),
    # Props found in any zone.
    ("PropBookshelf", "prop_bookshelf.png", 32, True),
    ("PropCamp", "prop_camp.png", 32, True),
    ("PropLockedChest", "prop_lockedchest.png", 32, True),
    # Things to smash, in any zone.
    ("PropPot", "prop_pot.png", 32, True),
    ("PropPotUrn", "prop_pot_urn.png", 32, True),
    ("PropPotJar", "prop_pot_jar.png", 32, True),
    ("PropCrate", "prop_crate.png", 32, True),
    # One more for each zone, and two more for any.
    ("PropStatue", "prop_statue.png", 32, True),
    ("PropGrave", "prop_grave.png", 32, True),
    ("PropBasket", "prop_basket.png", 32, True),
    ("PropCart", "prop_cart.png", 32, True),
    ("PropCage", "prop_cage.png", 32, True),
    ("PropOldChest", "prop_oldchest.png", 32, True),
    ("PropMirror", "prop_mirror.png", 32, True),
    # Things once they are used up (tools/make_used.py makes most of them).
    ("PropBushUsed", "prop_bush_used.png", 32, True),
    ("PropStatueUsed", "prop_statue_used.png", 32, True),
    ("PropCandlesUsed", "prop_candles_used.png", 32, True),
    ("PropCoffinUsed", "prop_coffin_used.png", 32, True),
    ("PropCrystalUsed", "prop_crystal_used.png", 32, True),
    ("PropMushroomsUsed", "prop_mushrooms_used.png", 32, True),
    ("PropClamUsed", "prop_clam_used.png", 32, True),
    ("PropBasketUsed", "prop_basket_used.png", 32, True),
    ("PropRackUsed", "prop_rack_used.png", 32, True),
    ("PropCampUsed", "prop_camp_used.png", 32, True),
    ("PropCageUsed", "prop_cage_used.png", 32, True),
    ("PropChestOpen", "prop_chest_open.png", 32, True),
    ("FountainDry", "fountain_dry.png", 32, True),
    ("TorchA", "torch_a.png", 32, True),
    ("TorchB", "torch8_south.png", 32, True),
    ("Dust", "particle_dust.png", 8, True),
    ("Hand", "hand.png", 64, True),
    ("HandAxe", "hand_axe.png", 64, True),
    ("HandSpear", "hand_spear.png", 64, True),
    ("HandDagger", "hand_dagger.png", 64, True),
    ("HandHammer", "hand_hammer.png", 64, True),
]

# Things drawn from eight sides: the entry's own texture is the front view,
# <Name>View1..7 the others, in CrawlerGame::ViewIndex's order (front, front
# right, right, back right, back, back left, left, front left). Files:
# <stem>8_<direction>.png.
VIEWS = {
    "TorchA": "torch",
}
VIEW_DIRECTIONS = ["south", "south-east", "east", "north-east", "north", "north-west", "west", "south-west"]


def with_views(textures):
    out = []
    for name, file, size, sprite in textures:
        stem = VIEWS.get(name)
        if not stem:
            out.append((name, file, size, sprite))
            continue
        out.append((name, f"{stem}8_south.png", size, sprite))
        out += [(f"{name}View{i}", f"{stem}8_{d}.png", size, sprite) for i, d in enumerate(VIEW_DIRECTIONS) if i > 0]
    return out


TEXTURES = with_views(TEXTURES)

def to565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


# Walls, floors and ceilings: this many colours each, padded, so any of them
# shares a palette layout.
SURFACE_COLOURS = 32


def pixels_of(image):
    return list(image.get_flattened_data() if hasattr(image, "get_flattened_data") else image.getdata())


def indexed(image, sprite):
    """The picture as (indices, colours). A sprite's index 0 is transparent."""
    pixels = pixels_of(image)
    colours = []
    lookup = {}
    indices = []
    for r, g, b, a in pixels:
        if sprite and a < 128:
            indices.append(0)
            continue
        key = (r, g, b)
        if key not in lookup:
            lookup[key] = len(colours) + (1 if sprite else 0)
            colours.append(key)
        indices.append(lookup[key])
    return indices, colours


def shade(colour, level, tints):
    near, far = tints
    f = level / (LEVELS - 1)
    k = 1.0 - (1.0 - FLOOR_BRIGHTNESS) * f
    tint = [near[i] + (far[i] - near[i]) * f for i in range(3)]
    rr, gg, bb = (min(255, int(c * k * t)) for c, t in zip(colour, tint))
    value = to565(rr, gg, bb)
    return value ^ 1 if value == TRANSPARENT else value  # one blue step off, so it is drawn


def main():
    art = Path(sys.argv[1] if len(sys.argv) > 1 else r"C:\dkt\pocket-dungeon-art\raw")
    out = Path(__file__).resolve().parent.parent / "src" / "render" / "Textures.gen.h"
    lines = [
        "#pragma once",
        "",
        "// Generated by tools/bake_textures.py from the PixelLab art - do not edit.",
        f"// Each texture: one colour index per pixel and {LEVELS} RGB565 palettes, brightest first;",
        "// a sprite's index 0 is transparent (0xF81F).",
        "",
        "#include \"Raycaster.h\"",
        "",
        "#include <cstdint>",
        "",
        "namespace Dungeon::Textures",
        "{",
        "",
        f"constexpr int kLevels = {LEVELS};",
        "",
    ]
    total = 0
    for name, file, size, sprite in TEXTURES:
        image = Image.open(art / file).convert("RGBA")
        if image.size != (size, size):
            raise SystemExit(f"{file} is {image.size}, expected {size}x{size}")
        base = name.rstrip("0123456789")
        zone = next((z for z in RECOLOUR if base.endswith(z) and name.startswith(SURFACE_NAMES)), None)
        if zone:
            colour, amount, scale = RECOLOUR[zone]
            recoloured = []
            for r, g, b, a in pixels_of(image):
                grey = 0.30 * r + 0.59 * g + 0.11 * b
                recoloured.append(tuple(min(255, int((c * (1 - amount) + grey * k * amount) * scale))
                                        for c, k in zip((r, g, b), colour)) + (a,))
            image = Image.new("RGBA", image.size)
            image.putdata(recoloured)
        if sprite and not name.startswith(FLOATING):
            # Standing on the floor: the picture moved down to the image's
            # bottom edge, which the raycaster puts on the floor line.
            box = image.getchannel("A").getbbox()
            if box and box[3] < size:
                grounded = Image.new("RGBA", image.size, (0, 0, 0, 0))
                grounded.paste(image, (0, size - box[3]))
                image = grounded
        if name in SCALED:
            k = SCALED[name]
            alpha = image.getchannel("A")
            image = Image.eval(image.convert("RGB"), lambda v: min(255, int(v * k))).convert("RGBA")
            image.putalpha(alpha)
        if sprite:
            limit = COLOURS_BY_NAME.get(name, SPRITE_COLOURS.get(size, 24))
            if name in UNLIT:
                limit = 24
        else:
            limit = SURFACE_COLOURS
        image = reduce_colours(image, limit)
        indices, colours = indexed(image, sprite)
        if not sprite:
            colours += [colours[-1]] * (SURFACE_COLOURS - len(colours))
        tints = next((t for zone, t in ZONE_TINTS.items() if name.rstrip("0123456789").endswith(zone)), (NEAR_TINT, FAR_TINT))
        count = 1 if name in UNLIT else LEVELS
        palette = []
        for level in range(count):
            if sprite:
                palette.append(TRANSPARENT)
            palette += [shade(c, level, tints) for c in colours]
        width = len(colours) + (1 if sprite else 0)
        total += len(indices) + 2 * len(palette)
        lines.append(f"inline constexpr uint8_t k{name}Index[] = {{")
        for i in range(0, len(indices), 32):
            lines.append("    " + ", ".join(str(v) for v in indices[i:i + 32]) + ",")
        lines.append("};")
        lines.append(f"inline constexpr uint16_t k{name}Palette[] = {{")
        for i in range(0, len(palette), 16):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in palette[i:i + 16]) + ",")
        lines.append("};")
        lines.append(f"inline constexpr Texture k{name}Texture{{ k{name}Index, k{name}Palette, {size}, {count}, {width} }};")
        lines.append("")
    lines.append("}  // namespace Dungeon::Textures")
    out.write_text("\n".join(lines) + "\n", newline="\n")
    print(f"wrote {out} ({len(TEXTURES)} textures, {LEVELS} levels, {total // 1024} KB)")


if __name__ == "__main__":
    main()
