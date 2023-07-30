export class Go2RTCMJpegPlayer extends HTMLElement {

	constructor() {
		super();

		this.wrapper = null;
		this.img = null;
		this.timestamp = null;
		this._src = null;
		this.initialized = false;

		this.playing = false;
		this.playPause = null;
		this.controls = null;

		this.ws = null;
	}

	static get observedAttributes() {
		return ['src'];
	}

	connectedCallback() {

		if ( ! this.initialized ){

			this.attachShadow({ mode: "open" }); 

			if ( !this.wrapper ){
				this.wrapper = document.createElement("div");
				this.wrapper.classList.add("wrapper");
				
				const style = document.createElement("style");
				style.textContent = this.getStyles();

				this.shadowRoot.append(style, this.wrapper);
			}

			if ( !this.img ){
				this.img = document.createElement("img");
                console.log("set style=", this._imgStyle);
				this.wrapper.append(this.img);

				this.controls = document.createElement("div");
				this.controls.classList.add("controls");

				this.playPause = document.createElement("button");
				this.playPause.classList.add("playPause");

				this.playIcon = document.createElement("span");
				this.playIcon.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="currentColor" d="M8 5.14v14l11-7l-11-7Z"/></svg>`;
				// this.playPause.append(this.playIcon);

				this.pauseIcon = document.createElement("span");
				this.pauseIcon.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="currentColor" d="M14 19h4V5h-4M6 19h4V5H6v14Z"/></svg>`;
				// this.playPause.append(pauseIcon);

				this.playPause.onclick = () => {
					this.onPlayPauseClick();
				}
				this.controls.append(this.playPause);

				this.timestamp = document.createElement("div");
				this.timestamp.classList.add("timestamp");
				this.controls.append(this.timestamp);

				this.wrapper.append(this.controls);
			}

			// if we have an initial src, handle it
			if ( this.src && this.autoplay ){
				this.onSrcChanged(this.src);
			} else {
                this.stop();
            }

			this.initialized = true;
		}
		
	}

	attributeChangedCallback(attrName, oldVal, newVal) {
		console.log("attribute changed ", attrName, oldVal, newVal);
		
		if ( this.initialized ){
			if ( attrName == "src" ){
				if ( newVal != this._src ){
					this._src = newVal;
					this.onSrcChanged(this._src);	
				}
			} 
		}

	}

    get autoplay(){
        return this.getAttribute("autoplay");
    }

	get src() {
		return this.getAttribute("src");
	}
	
	set src(value) {
		this.setAttribute("src", value);
	}

	onSrcChanged(src){
		console.log("on src changed ", src);

		this.play(src);

	}

	onPlayPauseClick(){

		if ( this.playing ){
			this.stop();
		} else {
			this.play(this.src);
		}
	}

	play(src){
		if ( this.ws ){
			this.ws.close();
		}

		this.timestamp.innerText = "loading...";

		this.ws = new WebSocket(src);

		// setting these directly messses up 'this', but chaining these with a lambda makes it work.. 
		this.ws.onopen = () => { this.wsOnOpen() };
		this.ws.onmessage = (msg) => { this.wsOnMessage(msg) };

		this.playing = true;
		this.playPause.replaceChildren(this.pauseIcon);
	}

	stop(){
		if ( this.ws ){
			this.ws.close();
		}
		this.playing = false;
		this.playPause.replaceChildren(this.playIcon);
	}

	wsOnOpen() {
		console.log("ws open");
		this.ws.binaryType = 'arraybuffer';
		this.ws.send(JSON.stringify({type: "mjpeg"}));
	}

	wsOnMessage(msg){
		if ( typeof(msg.data) === 'string' ){
			console.log("message", msg);
		} else {
			this.img.src = 'data:image/jpeg;base64,' + this.btoa(msg.data);

			this.timestamp.innerText = this.getDateString();

		}
	}

	getDateString(){
		const d = new Date();
		return `${d.getFullYear()}-${this.leftPad(d.getMonth()+1,2)}-${this.leftPad(d.getDate(),2)} ${this.leftPad(d.getHours(),2)}:${this.leftPad(d.getMinutes(), 2)}:${this.leftPad(d.getSeconds(), 2)}`
	}

	leftPad(val,width){
		let rval = "" + val;
		while ( rval.length < width ){
			rval = "0" + rval;
		}
		return rval;
	}


	btoa(buffer) {
	    const bytes = new Uint8Array(buffer);
	    const len = bytes.byteLength;
	    let binary = '';
	    for (let i = 0; i < len; i++) {
	        binary += String.fromCharCode(bytes[i]);
	    }
	    return window.btoa(binary);
	}


	getStyles(){
		return `
:host {
	width: 100%;
	height: 100%;
	margin: 0;
	padding: 0;
	display: block;
}

.wrapper {
	width: 100%;
	height: 100%;
}

.controls {
	display: flex;
	justify-content: space-between;
	padding-top: 2px;
	vertical-align: middle;
	align-items: center;
	height: 28px;
    padding-left: 8px;
    padding-right: 8px;
}

button {
	border: none;
	background: none;
	color: #777;
}

button:hover {
	color: #000000;
	cursor: pointer;
}

button:active {
	color: blue;
}

img {
	background-color: black;
	width: 100%;
	height: calc(100% - 30px);
	display: block;
    aspect-ratio: var(--go2rtc-aspect-ratio, 16/9);
}

.timestamp{
	vertical-align: middle;
}
		`;
	}

}

customElements.define("go2rtc-mjpeg-player", Go2RTCMJpegPlayer);

export class BambuP1SStreamerCard extends HTMLElement {

    set hass(hass) {
        // this is called frequently with changes to the overall state.
    }
    
    setConfig(config){
        this.config = config;

        if ( !config.src ){
            throw Error("src config is required");
        }

        let styleAttr = "";
        if ( this.config.aspectRatio ){
            styleAttr = `style="--go2rtc-aspect-ratio: ${this.config.aspectRatio}"`
        }

        // just set the innerHTML when the config is set, it's not dependent on hass state data.
        this.innerHTML = `
            <ha-card>
                <div style="border-radius: var(--ha-card-border-radius,12px); overflow: hidden;">
                    <go2rtc-mjpeg-player src="${config.src}" ${styleAttr}></go2rtc-mjpeg-player>
                </div>
            </ha-card>
        `;
    }
    
    getCardSize(){
        return 1;
    }

}

customElements.define("bambu-p1s-streamer-card", BambuP1SStreamerCard);
