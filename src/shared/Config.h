// Document Frequency (DF) cutoff
#define DF_CUTOFF 9

// Default vocabulary size
#define DEFAULT_VOCAB_SIZE 33554432

// Default number of documents in the collection
#define DEFAULT_COLLECTION_SIZE 30000000

// Inverted index file name
#define INDEX_FILE "index"

// Dictionary pointers file name
#define POINTER_FILE "pointers"

// Dictionary file name
#define DICTIONARY_FILE "dictionary"

// Document vectors file name
#define DOCUMENT_VECTOR_FILE "vectors"

// Number of pools in segment pool
#define NUMBER_OF_POOLS 4

// Buffer expansion rate for buffer maps
#define EXPANSION_RATE 2

// Index type (non-positional, docids and tf, and positional)
#define NONPOSITIONAL 0
#define TFONLY 1
#define POSITIONAL 2

