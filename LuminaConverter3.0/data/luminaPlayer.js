
let player;
let currentTrack;
let loadedTracks = [];
let jsmediatags;
let noSleep;

window.addEventListener('DOMContentLoaded', () => {
    player = document.getElementById('player');
    jsmediatags = window.jsmediatags;
    noSleep = new NoSleep();
    const addFilesBtn = document.getElementById("addFilesBtn");
    const fileInput = document.getElementById("fileInput");
    const playlistElement = document.querySelector(".aside");
    const playerProgress = document.getElementById("progresSlider");
    const volumeSlider = document.getElementById("volumeSlider");
    const prevBtn = document.getElementById("prevBtn");
    const playBtn = document.getElementById("playBtn");
    const nextBtn = document.getElementById("nextBtn");

    initWebSocket();

    addFilesBtn.addEventListener("click", async () => {
        fileInput.click();
    });

    fileInput.addEventListener("change", () => {
        let files = Array.from(fileInput.files || []);
        files = files.filter(f => f.type.startsWith("audio/"));
        addTracks(files, playlistElement);
    });

    player.addEventListener("timeupdate", () => {
        let currentTime = player.currentTime;
        let duration = player.duration;
        playerProgress.value = currentTime * 100 / duration;
        playBtn.textContent = !player.paused ? "Pause" : "Play";
        if (player.ended) nextBtn.click();
    });

    player.addEventListener("play", async () => {
        initAudio();
        if (ctx.state == "suspended") await ctx.resume();
    });

    volumeSlider.addEventListener("input", () => {
        player.volume = volumeSlider.value / 100;
    });

    playerProgress.addEventListener("input", () => {
        player.currentTime = playerProgress.value / 100 * player.duration;
    });

    playBtn.addEventListener("click", () => {
        if (player.src == "") {
            loadedTracks[0].click();
        } else {
            if (player.paused || player.ended) player.play(); else player.pause();
        }
    });

    nextBtn.addEventListener("click", () => {
        let currentIndex = loadedTracks.indexOf(currentTrack);
        if (currentIndex >= 0) {
            currentIndex = currentIndex + 1 == loadedTracks.length ? 0 : currentIndex + 1;
            currentTrack = loadedTracks[currentIndex];
            currentTrack.click();
        }
    });

    prevBtn.addEventListener("click", () => {
        let currentIndex = loadedTracks.indexOf(currentTrack);
        if (currentIndex >= 0) {
            currentIndex = currentIndex - 1 < 0 ? loadedTracks.length - 1 : currentIndex - 1;
            currentTrack = loadedTracks[currentIndex];
            currentTrack.click();
        }
    });

    document.addEventListener('click', function enableNoSleep() {
        document.removeEventListener('click', enableNoSleep, false);
        noSleep.enable();
    }, false);
});

function createTrack(id, title, file) {
    const trackBox = document.createElement("div");
    trackBox.className = "track";
    //trackBox.dataset.id = id;
    const trackTitle = document.createElement("span");
    trackTitle.className = "trackLabel";
    trackTitle.textContent = title;
    const trackLength = document.createElement("span");
    trackLength.className = "trackLength";
    const length = setDuration(file, trackLength);

    trackBox.addEventListener("click", () => {
        document.querySelectorAll(".track").forEach(track => {
            track.classList.remove("selected");
        });
        trackBox.classList.add("selected");
        playFile(URL.createObjectURL(file));
        currentTrack = trackBox;
        readFile(file);

        const list = document.querySelector(".asideList");
        const selected = document.querySelector(".track.selected");
        revealIn(list, selected, trackBox.offsetHeight);
    });

    trackBox.append(trackTitle, trackLength);
    loadedTracks.push(trackBox);
    return trackBox;
}

function addTracks(files, playlistElement) {
    const tracks = document.createDocumentFragment();
    for (let i = 0; i < files.length; i++) {
        console.log(files[i].name);
        tracks.appendChild(createTrack(i, files[i].name, files[i]));
    }
    const prevPlaylists = document.getElementsByClassName("asideList");
    const playListDiv = prevPlaylists.length == 0 ? document.createElement("div") : prevPlaylists[0];
    playListDiv.className = "asideList";
    playListDiv.append(tracks);
    playlistElement.appendChild(playListDiv);
}

async function playFile(path) {
    player.src = path;
    try { await player.play(); }
    catch (err) { console.warn('Playback was blocked!', err); }
}

function readFile(file) {
    jsmediatags.read(file, {
        onSuccess: function(tag) {
            const picture = tag.tags.picture;
            const imgElement = document.getElementById("coverImg");
            if (picture && picture.data && picture.format) {
                const data = picture.data;
                const format = picture.format;

                let binary = "";
                for (let i = 0; i < data.length; i++) {
                    binary += String.fromCharCode(data[i]);
                }
                const base64Data = window.btoa(binary);
                imgElement.classList.remove("hidden");
                imgElement.src = `data:${format};base64,${base64Data}`;
            } else {
                if (!imgElement.classList.contains("hidden")) imgElement.classList.add("hidden");
                console.warn("No embedded picture found in tags.");
            }

            console.log(tag.tags.title);
            document.getElementById("trackName").textContent = tag.tags.title;
            console.log(tag.tags.artist);
            document.getElementById("artist").textContent = tag.tags.artist;
            console.log(tag.tags.album);
            console.log(tag.tags.genre);
        },
        onError: function(error) {
            console.log(error);
        }
    });
}

async function setDuration(file, targetElement) {
    const seconds = await getAudioDurationWebAudio(file);
    targetElement.textContent = formatTime(seconds);
}

async function getAudioDurationWebAudio(file) {
    const ctx = getAudioDurationWebAudio.ctx || (getAudioDurationWebAudio.ctx = new (window.AudioContext || window.webkitAudioContext)());
    const buf = await file.arrayBuffer();
    const audioBuf = await ctx.decodeAudioData(buf.slice(0));
    return audioBuf.duration;
}

function formatTime(seconds) {
    if (!isFinite(seconds)) return "--:--";
    const m = Math.floor(seconds / 60);
    const s = Math.floor(seconds % 60).toString().padStart(2, "0");
    return `${m}:${s}`;
}

function revealIn(container, elem, offset = 0) {
    const cTop = container.scrollTop;
    const relTop = elem.getBoundingClientRect().top - container.getBoundingClientRect().top;
    const target = cTop + relTop - (container.clientHeight - elem.clientHeight) / 2 - offset;
    const max = container.scrollHeight - container.clientHeight;
    container.scrollTo({ top: Math.max(0, Math.min(target, max)), behavior: 'smooth' });
}