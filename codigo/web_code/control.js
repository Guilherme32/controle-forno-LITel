document.addEventListener("DOMContentLoaded", function(event) {
    document.querySelector("#operation_info").onsubmit = submit_operation;

    document.querySelector("#mode_set_point").onchange = update_operation_mode;
    document.querySelector("#mode_power").onchange = update_operation_mode;
    document.querySelector("#mode_set_point").checked = true;
    update_operation_mode();

    var plot_data = [
        ["s1", ],
        ["s2", ],
        ["s3", ],
        ["s4", ],
        ["s5", ],
        ["s6", ],
        ["set_point", ],
        ["tempo", ]
    ];

    const init_time = Date.now();
    var chart = start_chart(plot_data);

    setInterval(() => {update_interface(init_time, plot_data, chart)}, 5000);

    // Buf Fix. O grafico não atualizava o tamanho. Isso corrige
    window.addEventListener("resize", () => {
        const graph_container = document.querySelector("#graph_container");
        var size = { height: graph_container.offsetHeight - 10 };
        chart.resize(size);
    });

});

function reading_to_temp(reading){
    const voltage = (reading * 2) / 1023;
    const temp = voltage / 0.01;
    return temp;
}

function update_operation_mode(){
    // Chamado quando o usuario escolhe um método de funcionamento para atualizar essa
    // parte da pagina

    if(document.querySelector("#mode_set_point").checked){
        document.querySelector("#set_point_div").classList = "";
        document.querySelector("#power_div").classList = "disabled";
    } else {
        document.querySelector("#set_point_div").classList = "disabled";
        document.querySelector("#power_div").classList = "";
    }
}

function submit_operation(){
    // Envia as informacoes de operacao para o esp. Basicamente decide o modo e chama
    // a funcao para enviar o correspondente

    if(document.querySelector("#mode_set_point").checked){
        return submit_set_point();
    } else {
        return submit_power();
    }
}

function submit_set_point(){
    // Envia o set point novo para o sistema de controle

    const set_point_temp = parseFloat(document.querySelector("#set_point_div input").value);
    const voltage = set_point_temp * 0.01;
    const set_point_reading = 1023 * voltage / 2;

    if(set_point_reading === NaN){
        alert("Digite um número");
        return false;
    }

    const request = new Request("/api/send_set_point", {
        method: "POST",
        body: JSON.stringify({
            set_point: set_point_reading
        })
    });

    fetch(request)
    .then((response) => {
        if(response.status != 200){
            alert("Envio falhou, status" + response.status);
        }
    }).catch((reason) => {
        alert("Envio falhou, não recebeu resposta do servidor");
    })

    return false;
}

function submit_power(){
    // Envia a potência nova para o sistema de controle

    var power = parseInt(document.querySelector("#power_div input").value);
    if(power === NaN){
        alert("Digite um número");
        return false;
    }

    power = Math.round(power * 120 / 100)

    const request = new Request("/api/send_power", {
        method: "POST",
        body: JSON.stringify({
            power: power
        })
    })

    fetch(request)
    .then((response) => {
        if(response.status != 200){
            alert("Envio falhou, status" + response.status);
        }
    }).catch((reason) => {
        alert("Envio falhou, não recebeu resposta do servidor");
    })

    return false;
}

function update_interface(init_time, plot_data, chart){
    // Adquire algumas informacoes do servidor para atualizar a interface. Atualiza a
    // potencia enviada a carga, o estado do sistema, a temperatura no sensor principal,
    // e o grafico

    const request = new Request("api/controller_info", {
        method: "get"
    });

    fetch(request)
    .then((response) => response.json())
    .then((result) => {
        // Parte superior da página
        if(result["steady_state"]){
            document.querySelector("#steady_state").innerHTML = "Estacionário";
        } else {
            document.querySelector("#steady_state").innerHTML = "Transitório";
        }

        const power = Math.round(100 * result["power_ratio"][0]
                                            / (result["power_ratio"][0] + result["power_ratio"][1]));
        document.querySelector("#power_bar").style.width = power + "%";
        document.querySelector("#power").innerHTML = power + "%";

        const temp_s5 = reading_to_temp(result["readings"][4]);
        document.querySelector("#temp_s5").innerHTML = temp_s5.toFixed(2);

        // Gráfico
        // 240 -> 20 min
        if(plot_data[0].length > 240){
            for(var i=0; i < 8; i++){
                plot_data[i].splice(1, 1);
            }
        }

        for(var i=0; i<6; i++){
            plot_data[i].push(reading_to_temp(result["readings"][i]));
        }

        if(result["control"]){
            plot_data[6].push(reading_to_temp(result["set_point"]));
        } else {
            plot_data[6].push(NaN);
        }
        plot_data[7].push((Date.now() - init_time)/1000);

        chart.load({
            columns: plot_data,
            x: "tempo"
        })
    }).catch((reason) => {
        console.log("Não obteve resposta do servidor." + reason);
    })
}

function start_chart(init_data){
    // Inicializa o grafico sem informacoes

    var chart = c3.generate({
        bindto: "#graph",
        data: {
            columns: init_data,
            x: "tempo"
        },
        axis: {
            x: {
                label: {
                    text: "Tempo (s)",
                    position: "outer-middle"
                },
                tick: {
                    format: d3.format(".0f")
                }
            },
            y: {
                label: {
                    text: "Temperatura (ºC)",
                    position: "outer-middle"
                },
                tick: {
                    format: d3.format(".1f")
                }
            }
        }
    });

    return chart;
}
