Query File
------------

A query file should be in the following format:

	<first_line> .=. <Number of queries:integer>
	<line> .=. <query id: integer> <query length: integer> <query: text>

Input Documents
--------------

The input to the index is a set of (gzipped or raw) text files. In each file, each line contains one document in the following format:

	<document_id: integer> \t <document: text>.

Please note that, you must perform necessary preprocessing (e.g., parsing, stopping, stemming) prior to using the indexer, as the indexer is only able to read parsed documents and does not perform any sort of stopping or stemming.

Building an Inverted Index
--------------

To run the indexer:

	out/driver/indexer -index <output-index-root-path> [-tf | -positional] [-reverse] [-vectors]
	[-bloom -r <bits-per-element> -k <number-of-hash-functions>]
	-mb <maximum-buffer-length-in-number-of-blocks> -input <input-paths>

* `-reverse` indicates that postings will be stored in reverse order, such that the last posting inserted into the index will be read first.

* By using `-vectors`, you can accumulate document vectors in addition to the inverted index--which can be used in feature extraction.

* Indicate whether or not the indexer should create Bloom filter chains with `-bloom`. If Bloom filter chains are enabled, you must provide `-r`, number of bits per element, and `-k`, number of hash functions, used to construct Bloom filter chains.

* `mb` is the maximum buffer length in buffer maps.

* `-input` must be the last argument, and that `<input-paths>` is a list of files.

You can post-process an inverted index to create a contiguous index as follows:

	out/driver/buildContiguous -input <input-index-root-path> -output <output-index-root-path>

Retrieval
----------------------

To perform retrieval:

	out/driver/retrieval -index <index-root-path> -query <query-path>
	-algorithm <SvS|WAND|MBWAND|BWAND_OR|BWAND_AND>
	[-features <feature-path>]
	[-model <tree-ensemble-path>]
	[-hits <hits>] [-output <output-path>]

* `-features` tells the driver to compute feature values for the top documents.
This can only be done if the index is augmented with document vectors.
`<feature-path>` is the path to the file that contains feature definitions.
Please see `sample/features` for a sample feature set.

* `model` is the path to a tree ensemble model. If a tree model is given,
documents are ranked using the tree model in the final stage of retrieval.
For formatting details, see http://nasadi.github.com/OptTrees.

* `-hits` is the number of hits (default value is 1000). If `-hits` is not provided for SvS or BWAND_AND, the entire intersection set is computed.

* If `-output` is included, the output is stored at `<output-path>`.
