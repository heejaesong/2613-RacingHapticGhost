import mmap

names = [
    "acpmf_physics",
    r"Local\acpmf_physics",
    "acpmf_graphics",
    r"Local\acpmf_graphics",
    "acpmf_static",
    r"Local\acpmf_static",
]

for name in names:
    try:
        mm = mmap.mmap(-1, 4096, tagname=name, access=mmap.ACCESS_READ)
        print("FOUND ✅", name)
        mm.close()
    except Exception as e:
        print("NOT FOUND ❌", name, "|", type(e).__name__, str(e))
