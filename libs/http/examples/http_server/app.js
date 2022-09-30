
function get_status() {
    fetch('status.json').then(resp => resp.json()).then(json => {
        document.querySelector("#status").innerText = JSON.stringify(json);
    });
}
