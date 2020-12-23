# nvapi-node

> Native C++ module for interfacing with [NVIDIA NVAPI](https://developer.nvidia.com/nvapi)

Supports Windows 10 x64.

## Install

```sh
$ npm i nvapi-node
```

## Usage

```js
const nvapi = require('nviapi-node')

nvapi.getDigitalVibrance() // { minDV: 0, maxDV: 63, currentDV: 0 }
nvapi.toggleDigitalVibrance() // 0|-1
```

## References

- [falahati/NvAPIWrapper/blob/master/NvAPIWrapper/Native/Helpers/FunctionId.cs](https://github.com/falahati/NvAPIWrapper/blob/master/NvAPIWrapper/Native/Helpers/FunctionId.cs)

## License

MIT © [Søren Brokær](https://srn.io)
