from pathlib import Path
import sys

# Ensure secp256k1lab is found and can be imported directly
sys.path.insert(0, str(Path(__file__).parent / "../src/"))
