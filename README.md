jaasl
=====

Just Another Android (native) Sound Library
It's ironic, because there aren't any others...

### Why? ###
Because I wanted to implement some simple audio in my fully-native Android app and nothing
seemed to be available apart from a bunch of C examples that were overly complicated
and didn't have the functionality I wanted. Hence JAASL.

### Limitations ###
This implementation is rather simple. It creates a file-descriptor player based on an Android
asset. It doesn't do anything clever like audio-pools or circular buffers or such. For basic
games though, this shouldn't be too much of a problem. Let's just say you should have less
than 10 audio clips to play. Also you can't concurrently play the same audio clip. Sorry
about that.

Hopefully this should be enough to get basic sound in your Android game, though.