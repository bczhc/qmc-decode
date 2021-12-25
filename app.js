// EOF Magic detection.
const DETECTION_SIZE = 40;

// Process in 2m buffer size
const DECRYPTION_BUF_SIZE = 2 * 1024 * 1024;

let lastURL = "";

const $prog = document.getElementById("prog");
const $input = document.getElementById("input");
const $dl = document.getElementById("download");
const $player = document.getElementById("player");

function setInProgress(inProgress) {
  $input.disabled = $player.disabled = inProgress;
  if (inProgress) {
    $player.pause();
  }
}

const getMagic = (u8) => {
  return u8.slice(0, 4).reduce((result, byte, position) => {
    return result | (byte << (position * 8));
  }, 0);
};

/**
 * 根据解密后的文件解析，获得新的文件名及对应 mimetype。
 * @param  {Uint8Array[]} u8Array 解密后的文件片段集合
 * @param  {string} fileName 原始文件名
 * @return {[string, string]} 新的文件名以及 mimetype。
 */
function fileDetection(u8Array, fileName) {
  const oggMagic = 0x5367674f;
  const flacMagic = 0x43614c66;

  const magic = getMagic(u8Array[0]);

  // 未能识别时的返回内容
  let ext = ".bin";
  let mimeType = "application/octet-stream";

  switch (magic) {
    case oggMagic:
      ext = ".ogg";
      mimeType = "audio/ogg";
      break;
    case flacMagic:
      ext = ".flac";
      mimeType = "audio/flac";
      break;
  }

  const newFileName = fileName.replace(/(\.[^.]+)?$/, ext);
  return [newFileName, mimeType];
}

/**
 * 解密一个 QMC2 加密的文件。
 *
 * 如果检测并解密成功，返回解密后的 Uint8Array 数组，按顺序拼接即可得到完整文件。
 * 若失败，返回 `null`。
 * @param  {ArrayBuffer} mggBlob 读入的文件 ArrayBuffer
 * @param  {string}         name 文件名
 * @return {Promise<Uint8Array[]|null>}
 */
async function decryptMGG(mggBlob) {
  // 初始化模组
  const QMCCrypto = await QMCCryptoModule();

  // 申请内存块，并文件末端数据到 WASM 的内存堆
  const detectionBuf = new Uint8Array(mggBlob.slice(-DETECTION_SIZE));
  const pDetectionBuf = QMCCrypto._malloc(detectionBuf.length);
  QMCCrypto.writeArrayToMemory(detectionBuf, pDetectionBuf);

  // 检测结果内存块
  const pDetectionResult = QMCCrypto._malloc(QMCCrypto.sizeof_qmc_detection());

  // 进行检测
  const detectOK = QMCCrypto.detectKeyEndPosition(
    pDetectionResult,
    pDetectionBuf,
    detectionBuf.length
  );

  // 提取结构体内容：
  // (pos: i32; len: i32; error: char[??])
  const position = QMCCrypto.getValue(pDetectionResult, "i32");
  const len = QMCCrypto.getValue(pDetectionResult + 4, "i32");
  const detectionError = QMCCrypto.UTF8ToString(
    pDetectionResult + QMCCrypto.offsetof_error_msg(),
    QMCCrypto.sizeof_error_msg()
  );
  const songId = QMCCrypto.UTF8ToString(
    pDetectionResult + QMCCrypto.offsetof_song_id(),
    QMCCrypto.sizeof_song_id()
  );
  console.info("Deceted song id: %s", songId);

  // 释放内存
  QMCCrypto._free(pDetectionBuf);
  QMCCrypto._free(pDetectionResult);

  if (detectOK) {
    // 计算解密后文件的大小。
    // 之前得到的 position 为相对当前检测数据起点的偏移。
    const decryptedSize = mggBlob.byteLength - DETECTION_SIZE + position;
    $prog.max = decryptedSize;

    // 提取嵌入到文件的 EKey
    const ekey = new Uint8Array(
      mggBlob.slice(decryptedSize, decryptedSize + len)
    );

    // 解码 UTF-8 数据到 string
    const decoder = new TextDecoder();
    const ekey_b64 = decoder.decode(ekey);

    // 初始化加密与缓冲区
    const hCrypto = QMCCrypto.createInstWidthEKey(ekey_b64);
    const buf = QMCCrypto._malloc(DECRYPTION_BUF_SIZE);

    const decryptedParts = [];
    let offset = 0;
    let bytesToDecrypt = decryptedSize;
    while (bytesToDecrypt > 0) {
      const blockSize = Math.min(bytesToDecrypt, DECRYPTION_BUF_SIZE);

      // 解密一些片段
      const blockData = new Uint8Array(
        mggBlob.slice(offset, offset + blockSize)
      );
      QMCCrypto.writeArrayToMemory(blockData, buf);
      QMCCrypto.decryptStream(hCrypto, buf, offset, blockSize);
      decryptedParts.push(QMCCrypto.HEAPU8.slice(buf, buf + blockSize));

      offset += blockSize;
      bytesToDecrypt -= blockSize;
      $prog.value = offset;

      // 避免网页卡死，让 event loop 处理一下其它事件。
      await new Promise((resolve) => setTimeout(resolve));
    }
    QMCCrypto._free(buf);
    hCrypto.delete();

    return decryptedParts;
  } else {
    alert("ERROR: could not decrypt\n       " + detectionError);
    return null;
  }
}

/**
 * 处理一个文件。
 * @param  {File} 通过拖放或文件输入获得的文件。
 */
function processFile(file) {
  setInProgress(true);
  const fileName = file.name;
  const reader = new FileReader();
  reader.addEventListener("abort", () => {
    setInProgress(false);
  });
  reader.addEventListener("error", (err) => {
    setInProgress(false);
    console.error(err);
    alert("读取文件失败: " + err.message);
  });
  reader.addEventListener("load", (e) => {
    if (lastURL) {
      URL.revokeObjectURL(lastURL);
      lastURL = "";
    }

    decryptMGG(e.target.result)
      .then((decryptedParts) => {
        if (!decryptedParts) return;

        const [newFileName, mimeType] = fileDetection(decryptedParts, fileName);

        const blob = new Blob(decryptedParts, {
          type: mimeType,
        });

        const url = (lastURL = window.URL.createObjectURL(blob));
        $player.src = url;

        $dl.href = url;
        $dl.textContent = newFileName;
        $dl.download = newFileName;
      })
      .catch((err) => {
        console.error(err);
        alert("解密失败: \n" + err.message);
      })
      .then(() => {
        setInProgress(false);
      });
  });
  reader.readAsArrayBuffer(file);
}

function main() {
  setInProgress(true);

  ///// 加载 QMC2-Crypto 组件

  // 检测 WASM 支援并加载对应文件
  const backend = window.WebAssembly ? "wasm" : "legacy";
  const qmc2Script = document.createElement("script");
  qmc2Script.src = `./QMC2-${backend}.js`;
  qmc2Script.onload = () => {
    document.getElementById("qmc2-backend").textContent = backend;
    setInProgress(false);
  };
  qmc2Script.onerror = () => {
    alert("加载 QMC2-Crypto 模组失败。");
  };
  document.head.appendChild(qmc2Script);

  // 绑定选择文件事件。
  $input.onchange = () => {
    processFile($input.files[0]);
  };

  document.getElementById("btn_start").onclick = () => {
    $input.click();
  };

  ///// 文件拖放支持

  let dragCounter = 0;
  function updateDragCounter(delta) {
    dragCounter += delta;
    document.body.classList.toggle("dragging", dragCounter > 0);
  }

  function handleDragEffect(e) {
    if (e.dataTransfer.types.includes("Files")) {
      e.dataTransfer.dropEffect = "copy";
      e.preventDefault();
    } else {
      e.dataTransfer.effectAllowed = false;
      e.dataTransfer.dropEffect = "none";
    }
  }

  document.body.addEventListener("dragenter", (e) => {
    updateDragCounter(+1);
    handleDragEffect(e);
    e.preventDefault();
  });

  document.body.addEventListener("drop", (e) => {
    e.preventDefault();
    updateDragCounter(-1);
    processFile(e.dataTransfer.files[0]);
  });

  document.body.addEventListener("dragover", (e) => {
    handleDragEffect(e);
  });

  document.body.addEventListener("dragleave", () => {
    updateDragCounter(-1);
  });
}

main();
