## mp3recover
mp3recover is an mp3 file name recovery tool

For example, if you have a failing USB flash drive and you use photorec (or similar tools)
to recover it's content you might end up with a list of files that look like this:

	f11000444667.mp3
	f11000444668.mp3
	f11000444669.mp3
	f11000444670.mp3
	...

mp3recover will try to read ID3v1 metadata and find the song's title and artist, if it succeeds in
doing so you will end up with a list of files looking more like this:

	Artist - Song title.mp3
	Artist - Song title.mp3
	Artist - Song title.mp3
	...

For files with missing artist or title tags mp3recover will produce output similar to this:

	- for files with missing artist: Title <some number>.mp3
	- for files with missing title:  Artist <some number>.mp3
	
mp3recover will not remove or modify the original file.

### Usage
	$ mp3recover -i=<source directory> -o=<target directory>
		-i - directory containing files to be processed
		-o - output directory

### Download and build
	$ git clone https://github.com/markotikvic/mp3recover.git
	$ cd mp3recover
	$ make

### Dependencies
mp3recover relies on [Underbit Technologies libid3tag library](https://www.underbit.com/products/mad/) for recovering ID3v2 tags.

### License
See [license](LICENSE).
