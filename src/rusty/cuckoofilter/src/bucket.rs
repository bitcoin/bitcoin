pub const FINGERPRINT_SIZE: usize = 1;
pub const BUCKET_SIZE: usize = 4;
const EMPTY_FINGERPRINT_DATA: [u8; FINGERPRINT_SIZE] = [100; FINGERPRINT_SIZE];

// Fingerprint Size is 1 byte so lets remove the Vec
#[derive(PartialEq, Copy, Clone, Hash)]
pub struct Fingerprint {
    pub data: [u8; FINGERPRINT_SIZE],
}

impl Fingerprint {
    /// Attempts to create a new Fingerprint based on the given
    /// number. If the created Fingerprint would be equal to the
    /// empty Fingerprint, None is returned.
    pub fn from_data(data: [u8; FINGERPRINT_SIZE]) -> Option<Self> {
        let result = Self { data };
        if result.is_empty() {
            None
        } else {
            Some(result)
        }
    }

    /// Returns the empty Fingerprint.
    pub fn empty() -> Self {
        Self {
            data: EMPTY_FINGERPRINT_DATA,
        }
    }

    /// Checks if this is the empty Fingerprint.
    pub fn is_empty(&self) -> bool {
        self.data == EMPTY_FINGERPRINT_DATA
    }

    /// Sets the fingerprint value to a previously exported one via an in-memory copy.
    fn slice_copy(&mut self, fingerprint: &[u8]) {
        self.data.copy_from_slice(fingerprint);
    }
}

/// Manages `BUCKET_SIZE` fingerprints at most.
#[derive(Clone)]
pub struct Bucket {
    pub buffer: [Fingerprint; BUCKET_SIZE],
}

impl Bucket {
    /// Creates a new bucket with a pre-allocated buffer.
    pub fn new() -> Self {
        Self {
            buffer: [Fingerprint::empty(); BUCKET_SIZE],
        }
    }

    /// Inserts the fingerprint into the buffer if the buffer is not full.
    /// This operation is O(1).
    pub fn insert(&mut self, fp: Fingerprint) -> bool {
        for entry in &mut self.buffer {
            if entry.is_empty() {
                *entry = fp;
                return true;
            }
        }
        false
    }

    /// Deletes the given fingerprint from the bucket. This operation is O(1).
    pub fn delete(&mut self, fp: Fingerprint) -> bool {
        match self.get_fingerprint_index(fp) {
            Some(index) => {
                self.buffer[index] = Fingerprint::empty();
                true
            }
            None => false,
        }
    }

    /// Returns the index of the given fingerprint, if its found. O(1)
    pub fn get_fingerprint_index(&self, fp: Fingerprint) -> Option<usize> {
        self.buffer.iter().position(|e| *e == fp)
    }

    /// Returns all current fingerprint data of the current buffer for storage.
    pub fn get_fingerprint_data(&self) -> Vec<u8> {
        self.buffer
            .iter()
            .flat_map(|f| f.data.iter())
            .cloned()
            .collect()
    }

    /// Empties the bucket by setting each used entry to Fingerprint::empty(). Returns the number of entries that were modified.
    #[inline(always)]
    pub fn clear(&mut self) {
        *self = Self::new()
    }
}

impl From<&[u8]> for Bucket {
    /// Constructs a buffer of fingerprints from a set of previously exported fingerprints.
    fn from(fingerprints: &[u8]) -> Self {
        let mut buffer = [Fingerprint::empty(); BUCKET_SIZE];
        for (idx, value) in fingerprints.chunks(FINGERPRINT_SIZE).enumerate() {
            buffer[idx].slice_copy(value);
        }
        Self { buffer }
    }
}
