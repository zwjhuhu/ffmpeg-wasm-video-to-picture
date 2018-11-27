## 修改一个ffmpeg + wasm实现网页逐帧绘制视频的功能，只是测试用的，移动端看上去无法实时

### 使用的ffmpeg版本是n3.4.5

### 简单对ffmpeg进行编译只支持解码h264和h265
```bash
emconfigure ./configure --cc="emcc" --cxx="em++" --ar="emar" --prefix=$(pwd)/../dist --enable-cross-compile --target-os=none --arch=x86_32 --cpu=generic --enable-gpl --enable-version3 \
--disable-avdevice --disable-swresample --disable-postproc --disable-avfilter --disable-programs \
--disable-everything --enable-decoder=hevc --enable-decoder=h264 --enable-demuxer=m4v --enable-demuxer=mov --enable-demuxer=matroska \
--disable-ffplay --disable-ffprobe --disable-ffserver --disable-asm --disable-doc --disable-devices --disable-network \
--disable-hwaccels --disable-bsfs --disable-debug --disable-protocols --disable-parsers --disable-indevs --disable-outdevs 
```

### 编译cfile/web.c proccess.c process.h
这个是网页版的核心C代码，使用以下命令编译成wasm：
```bash
cd ../dist
export EMMAKEN_CFLAGS="-I./include" 
emcc web.c process.c ./lib/libavformat.a ./lib/libavcodec.a ./lib/libswscale.a ./lib/libavutil.a -Os -s WASM=1 -o index.html -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=16777216
```

这个会生成index.wasm和index.js，在html引入
```html
<script src="index.js"></script>
```
它就会自动加载index.wasm文件
