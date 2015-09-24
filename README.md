# node-bgpdump2

## Description

NodeJS binding of [bgpdump2](https://github.com/yasuhiro-ohara-ntt/bgpdump2).

## Installation

```zsh
npm install node-bgpdump2
```

## Usage

```js
var BGPDump = require('node-bgpdump2')
bgpdump = new BGPDump('path_to_rib.bz2')
console.log(bgpdump.lookup('8.8.8.8'))
```

## Limitations

* Only support IPv4
* Only lookup option implemented

## Copyright and License

Copyright (c) 2015 Shintaro Kojima. Code released under the [MIT license](LICENSE).


