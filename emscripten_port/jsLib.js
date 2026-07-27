mergeInto(LibraryManager.library, {
    supplyAnimationList: function(arrPtr, length) {
        var animationIdArr = Module.HEAP32.subarray(arrPtr / 4, arrPtr / 4 + length);
        console.log(animationIdArr);

        Module['animationArrayCallback'](animationIdArr);
    },
    supplyMeshIds: function(arrPtr, length) {
        var meshIdArr = Module.HEAP32.subarray(arrPtr / 4, arrPtr / 4 + length);
        console.log(meshIdArr);

        if (Module['meshIdArrayCallback']) {
            Module['meshIdArrayCallback'](meshIdArr);
        }
    },
    offerFileAsDownload : function(filename_ptr, filename_len) {
        let mime = "application/octet-stream";

        let filename = UTF8ToString(filename_ptr);

        let content = Module.FS.readFile(filename);
        var a = document.createElement('a');
        a.download = filename;
        a.href = URL.createObjectURL(new Blob([content], {type: mime}));
        a.style.display = 'none';

        document.body.appendChild(a);
        a.click();
        setTimeout(() => {
            document.body.removeChild(a);
            URL.revokeObjectURL(a.href);
        }, 2000);
    }
});
