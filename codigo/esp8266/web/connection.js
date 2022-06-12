document.addEventListener("DOMContentLoaded", function(event) {
    get_ap_info();
    setTimeout(() => get_sta_info(true), 500)   // Esperando pra n sobrecarregar o servidor

    setTimeout(() => setInterval(get_sta_info, 1000), 10000)
});


function get_ap_info(){
    const request = new Request("api/ap_info", {
        method: "get"
    });

    fetch(request)
    .then((response) => response.json())
    .then((result) => {
        document.querySelector("#ap_ssid").innerHTML = result["ssid"];
        document.querySelector("#ap_password").innerHTML = result["password"];
        document.querySelector("#ap_ip").innerHTML = result["ip"];
    });
}


function get_sta_info(get_ssid=false){
    const request = new Request("api/sta_info", {
        method: "get"
    });

    fetch(request)
    .then((response) => response.json())
    .then((result) => {
        if(get_ssid){
            document.querySelector("#sta_ssid").value = result["ssid"];
            document.querySelector("#sta_password").value = result["password"];
        }

        const ip_value = result["connected"] ? result["ip"] : "<span class='error'>Desconectado</span>";
        document.querySelector("#sta_ip").innerHTML = ip_value;
    });
}


function submit_sta_config(){
    const sta_ssid = document.querySelector("#sta_ssid").value;
    const sta_password = document.querySelector("#sta_password").value;

    const request = new Request("api/sta_info", {
        method: "POST",
        body: JSON.stringify({
            ssid: sta_ssid,
            password: sta_password
        })
    });

    fetch(request)
    .then((response) => response.json())
    .then((result) =>{
        console.log(result)
    })

    get_sta_info(true)
    return false;
}
