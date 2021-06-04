import paho.mqtt.client as mqtt    # Módulo MQTT
import time            # Módulo para trabalhar com tempo e conversões
import pymysql.cursors        # Módulo Mysql (Banco de Dados)
import json            # Módulo Json
import pymysql
import paho

################################ FUNÇÕES MYSQL ###############################
# Insere mensagens recebidas no banco de dados.
def send_mysql(msg_vector):
    # Inicia conexão com o Banco de Dados Mysql
    try:
        connection = pymysql.connect(host='localhost',
                             user='root',
                             password='',
                             db='testepi',
                             charset='utf8mb4',
                             cursorclass=pymysql.cursors.DictCursor)

    except:
        print("Não foi possível conectar-se ao banco de dados.")
        return 0

    
    try:
        with connection.cursor() as cursor:        
            sql = "INSERT INTO datas (economia, rele, diff_hour, hour) VALUES(%s, %s, %s, %s)"
            # Executa query Insert
            cursor.execute(sql,( msg_vector[0], msg_vector[1], msg_vector[2], msg_vector[3] ))
        # Confirma a transação realizada
        connection.commit()
        print("Dados inseridos: economia = "+str(msg_vector[0])+", rele = "+str(msg_vector[1])+" , diff_hour = "+str(msg_vector[2])+", hour = "+str(msg_vector[3]) )
    except:
        print("Falha ao inserir dados no banco de dados.")
    
    finally:
        # Encerra conexão com Mysql
        connection.close()


################################ FUNÇÕES MQTT ################################
# Define função de retorno de chamada ao conectar-se com o Broker.
def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker.")
    # Inscreve-se no tópico para receber mensagens.
    client.subscribe("energymeterpi3teste/send")

# Define função de retorno de chamada ao receber mensagem. 
def on_message(client, userdata, msg):
    # Converte mensagem em bytes para string
    msg_string=str(msg.payload.decode("utf-8","ignore"))
    # Desserializa string Json para dicionário Python
    dict_json=json.loads(msg_string)
    # Arredonda corrente para duas casas decimais
    Irms = round(dict_json["Economia"], 2)
    # Arredonda Potencia para duas casas decimais
    Power = round(dict_json["Status"],2)
    # Armazena diff_hour
    Diff_hour = int(dict_json['Diff_hour'])
    # Obs: Esta hora está no padrão época
    Hour_epoch = int(dict_json["Hour"])
    # Converte a hora do padrão época para o padrão data e hora do Mysql
    Hour = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(Hour_epoch))
    # Armazena dados formatados no vetor msg_formated
    msg_formated = [Irms, Power, Diff_hour, Hour] 
    # Função que insere os dados no Mysql
    send_mysql(msg_formated)
    
client = mqtt.Client()

# Define função de retorno de chamada após uma desconexão.
def on_disconnect(client, userdata, rc):
    client = mqtt.Client()
    if rc != 0:
        print("Desconexão MQTT Inesperada.")
    print("Reconectando-se ao Broker em 3 segundos...")
    time.sleep(3)
    client.connect("mqtt.eclipseprojects.io", 1883, 60)

# Instancia cliente MQTT.
client.on_connect = on_connect        # Define como callback a função on_connect
client.on_message = on_message        # Define como callback a função on_message
client.on_disconnect = on_disconnect    # Define como callback a função on_disconnect

# Inicia conexão MQTT com o Broker Mosquitto.
client.connect("mqtt.eclipseprojects.io", 1883, 60)

client.loop_forever()