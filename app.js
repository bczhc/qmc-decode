const DETECTION_SIZE = 40;

// Process in 2m buffer size
const DECRYPTION_BUF_SIZE = 2 * 1024 * 1024;

let lastURL = "";

const $prog = document.getElementById("prog");
const $input = document.getElementById("input");
const $dl = document.getElementById("download");
const $player = document.getElementById("player");

async function decryptMGG(mggBlob, name) {
  const QMCCrypto = await QMCCryptoModule();

  const detectionBlob = new Uint8Array(mggBlob.slice(-DETECTION_SIZE));

  const detectionBuf = QMCCrypto._malloc(detectionBlob.length);
  QMCCrypto.writeArrayToMemory(detectionBlob, detectionBuf);
  const detectionResult = QMCCrypto._malloc(QMCCrypto.sizeof_qmc_detection());
  const ok = QMCCrypto.detectKeyEndPosition(
    detectionResult,
    detectionBuf,
    detectionBlob.length
  );
  const position = QMCCrypto.getValue(detectionResult, "i32");
  const len = QMCCrypto.getValue(detectionResult + 4, "i32");
  const detectionError = QMCCrypto.UTF8ToString(detectionResult + 8);

  QMCCrypto._free(detectionBuf);
  QMCCrypto._free(detectionResult);

  if (ok) {
    const decryptedSize = mggBlob.byteLength - DETECTION_SIZE + position;
    const ekey = new Uint8Array(
      mggBlob.slice(decryptedSize, decryptedSize + len)
    );

    const decoder = new TextDecoder();
    const ekey_b64 = decoder.decode(ekey);
    const hCrypto = QMCCrypto.createInstWidthEKey(ekey_b64);

    $prog.max = decryptedSize;
    const decryptedParts = [];
    const buf = QMCCrypto._malloc(DECRYPTION_BUF_SIZE);
    let bytesToDecrypt = decryptedSize;
    let offset = 0;
    while (bytesToDecrypt > 0) {
      const blockSize = Math.min(bytesToDecrypt, DECRYPTION_BUF_SIZE);

      const blockData = new Uint8Array(
        mggBlob.slice(offset, offset + blockSize)
      );
      QMCCrypto.writeArrayToMemory(blockData, buf);

      QMCCrypto.decryptStream(hCrypto, buf, offset, blockSize);

      decryptedParts.push(QMCCrypto.HEAPU8.slice(buf, buf + blockSize));

      offset += blockSize;
      bytesToDecrypt -= blockSize;
      $prog.value = offset;

      await new Promise((resolve) => setTimeout(resolve));
    }
    QMCCrypto._free(buf);
    // TODO: free hCrypto

    const isOGG = /\.mgg\d?$/.test(name);
    const isFLAC = /\.mflac\d?$/.test(name);
    const newExt = isOGG ? "ogg" : isFLAC ? "flac" : "bin";
    const newFileName = `${name.replace(/[^.]+$/, "")}${newExt}`;
    return [
      decryptedParts,
      newFileName,
      newExt === "bin" ? "appli" : `audio/${newExt}`,
    ];
  } else {
    alert("ERROR: could not decrypt\n       " + detectionError);
    return [];
  }
}

async function main() {
  $input.onchange = () => {
    const file = $input.files[0];
    const name = file.name;
    const reader = new FileReader();
    reader.addEventListener("load", (e) => {
      if (lastURL) {
        URL.revokeObjectURL(lastURL);
      }

      decryptMGG(e.target.result, name)
        .then(([blobs, newFileName, mimeType]) => {
          if (!blobs) return;

          const blob = new Blob(blobs, {
            type: mimeType,
          });

          const url = (lastURL = window.URL.createObjectURL(blob));
          $player.src = url;

          $dl.href = url;
          $dl.textContent = newFileName;
          $dl.download = newFileName;
        })
        .catch((err) => {
          alert("解密失败。\n" + err.message);
          console.error(err);
        });
    });
    reader.readAsArrayBuffer(file);
  };
}

main();
