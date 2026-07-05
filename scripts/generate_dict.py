import urllib.request
import json
import struct
import os

# URL of the Webster's compact dictionary JSON
DICT_URL = "https://raw.githubusercontent.com/matthewreagan/WebstersEnglishDictionary/master/dictionary_compact.json"

def main():
    print("Downloading English dictionary database (~5.5MB)...")
    try:
        req = urllib.request.Request(
            DICT_URL, 
            headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}
        )
        with urllib.request.urlopen(req) as response:
            raw_data = response.read().decode('utf-8')
            data = json.loads(raw_data)
    except Exception as e:
        print(f"Error downloading dictionary: {e}")
        return

    print("Processing and sorting words...")
    # Filter out empty entries and sort alphabetically
    sorted_words = sorted([w for w in data.keys() if w.strip()])
    entry_count = len(sorted_words)
    print(f"Total words found: {entry_count}")

    # Layout offsets
    # 4 bytes for count, followed by index records of 36 bytes each
    current_payload_offset = 4 + entry_count * 36
    
    index_entries = []
    payload_bytes = bytearray()

    for word in sorted_words:
        # Normalize: strip, lowercase, limit to max 27 chars to fit 28-byte index slot
        clean_word = word.strip().lower()[:27]
        definition = data[word].strip()
        
        def_bytes = definition.encode('utf-8')
        def_len = len(def_bytes)

        index_entries.append({
            'word': clean_word,
            'offset': current_payload_offset,
            'length': def_len
        })
        payload_bytes.extend(def_bytes)
        current_payload_offset += def_len

    output_path = "dict.bin"
    print(f"Writing packed binary index database to '{output_path}'...")
    
    with open(output_path, "wb") as f:
        # Write entry count (uint32)
        f.write(struct.pack("<I", entry_count))
        
        # Write index table (each entry is 36 bytes)
        for entry in index_entries:
            word_bytes = entry['word'].encode('utf-8')
            # Pad word to exactly 28 bytes with nulls
            word_padded = word_bytes.ljust(28, b'\x00')
            f.write(struct.pack("<28sII", word_padded, entry['offset'], entry['length']))
            
        # Write definitions payload
        f.write(payload_bytes)

    size_mb = os.path.getsize(output_path) / (1024 * 1024)
    print("Success!")
    print(f"Generated file size: {size_mb:.2f} MB")
    print("----------------------------------------------------------------------")
    print(f"Please copy the generated '{output_path}' file to your Cardputer's SD card at:")
    print("  /5herbetPDA/dict.bin")
    print("----------------------------------------------------------------------")

if __name__ == "__main__":
    main()
