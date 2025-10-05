- Precisa de 2 smartphones android, 1 para o professor e 1 para o aluno.
- Não testei com a versão desktop, meu adaptador bluetooth está com problemas.
- Relatos na internet falam que o BLE não funciona corretamente em emuladores.

**Como funciona:**
  **Lado do estudante**
   - O estudante irá precisar do Código da Sessão gerado pelo professor
     - Estava tendo vários problemas com a questão se sincronia de tempo entre a transmissão do professor e do aluno;
     - Com esse código, essa questão sempre vai ficar ancorada no professor
     - Se o aluno não tiver esse código gerado pelo professor, o Scanner não o identificará
  <img width="481" height="749" alt="image" src="https://github.com/user-attachments/assets/b835d013-ba54-4d7a-8ff9-0506c593b4f9" />
  
   - Com o código inserido, ele pode iniciar a transmissão no botão "Iniciar anúncio"
  <img width="479" height="749" alt="image" src="https://github.com/user-attachments/assets/34c24dfd-953c-4d73-bbad-77495f512399" />
  
  **Lado do professor:**
   - O professor gera o Código de Sessão e passa para os alunos
  <img width="481" height="749" alt="image" src="https://github.com/user-attachments/assets/824efb19-2b04-40f4-a85c-1efe4f5d563c" />
  
   - Então ele pode inicar o processo de scan (somente 1x ou de forma contínua)
  <img width="485" height="746" alt="image" src="https://github.com/user-attachments/assets/3560b948-6608-4e0c-b8c7-0452c41b36be" />

**Outras configs na versão atual desse app**
<img width="484" height="754" alt="image" src="https://github.com/user-attachments/assets/6d9b83bc-c904-422f-869d-cc64ed6a794e" />
 1. Secret ID do estudante
    - É a chave secreta individual de cada aluno.
    - Calculado como:
      - HMAC-SHA256(chave = secret_do_aluno, mensagem = le16(courseId) || nonceSessao || le32(slot))
      - Sendo:
        - courseId -> ID da classe
        - nonceSessao -> é um valor público que entra na mensagem do HMAC
            - Separa/namespaceia sessões e cursos, evitando que um mesmo secret gere o mesmo rollingId em contextos diferentes.
        - slot -> É o índice da janela de tempo dentro da sessão
            - Serve para o identificador do aluno mudar periodicamente (privacidade) e para “prender” todos à mesma linha do tempo da sessão. Mudou o slot → muda o HMAC → muda o rollingId
              - rollingId -> é o identificador rotativo (pseudônimo) que o aluno anuncia e que o professor usa como “pista” para encontrar quem é — sem expor a matrícula nem um ID fixo.
            - Calculado como:
              - slot = floor((nowEpochSec - sessionStartEpochSec) / rollingPeriodSec)
              - Sendo: 
                - nowEpochSec -> É o tempo atual em segundos desde 1970-01-01 UTC (epoch).
                - sessionStartEpochSec -> é a âncora de tempo da sessão (em segundos epoch).
                  - Calculado como: 
                    - sessionStartEpochSec = floor(now / rollingPeriodSec) * rollingPeriodSec
                  - Dessa forma, mesmo com relógios um pouco diferentes, todos começam na mesma borda e o scanner ainda tem tolerância de ±N slots (±2 atualmente) para cobrir drift/latência.
 2. ID da classe -> A ideia é pegar direto do SUAP
 3. Tempo para gerar uma nova Secret ID (rollingPeriodSec)

<img width="484" height="754" alt="image" src="https://github.com/user-attachments/assets/3893fb2a-d3c5-413a-b2e2-306f0ec2f539" />

1. Carrega alguns alunos padrões
 -  { id: "aluno01", secret: "segredo-aluno-001" } (id: alguma info relevante do aluno como o próprio ID, secret: Secret ID do estudante (gerada no app?))
 -  { id: "aluno02", secret: "segredo-aluno-002" }
 -  { id: "aluno03", secret: "segredo-aluno-003" }

2. Limpa a lista de alunos encontrados
3. ID da classe -> A ideia é pegar direto do SUAP
4. Range de detecção do bluetooth
5. Janela do auto escanear


**A fazer**
- Atuamente o Secret ID do estudante está fixo, tanto no aluno quanto no professor. Preciso ver como calcular isso com base no que vir do SUAP
- Interromper tanto o Advertiser quanto o Scan se o app for pro background. O BLE, no android, funciona melhor no foreground e no IOS só funciona assim
- Levar a parte em C++ pro Intermix? Como fazer? 
