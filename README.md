### Árvore de Integração/Teste do Bitcoin Core  
[https://bitcoincore.org](https://bitcoincore.org)  

Para uma versão binária imediatamente utilizável do software Bitcoin Core, acesse:  
[https://bitcoincore.org/en/download/](https://bitcoincore.org/en/download/).  

---

### O que é o Bitcoin Core?  
O Bitcoin Core conecta-se à rede peer-to-peer do Bitcoin para baixar e validar completamente blocos e transações. Ele também inclui uma carteira e uma interface gráfica (GUI), que pode ser construída opcionalmente.  

Mais informações sobre o Bitcoin Core estão disponíveis na pasta doc.

---

### Licença  
O Bitcoin Core é distribuído sob os termos da licença MIT. Veja o arquivo COPYING para mais detalhes ou acesse:  
[https://opensource.org/licenses/MIT](https://opensource.org/licenses/MIT).

---

### Processo de Desenvolvimento  
- O branch master é regularmente compilado (veja doc/build-*.md para instruções) e testado, mas sua estabilidade não é garantida.  
- Tags são criadas regularmente a partir de branches de lançamento para indicar novas versões oficiais e estáveis do Bitcoin Core.  
- O repositório [https://github.com/bitcoin-core/gui](https://github.com/bitcoin-core/gui) é exclusivamente usado para o desenvolvimento da interface gráfica (GUI). Ele compartilha o mesmo branch master com outros repositórios monotree. Não há branches ou tags de lançamento neste repositório, portanto, não o bifurque, a menos que seja para desenvolvimento.  

O fluxo de contribuição está descrito em CONTRIBUTING.md, e dicas úteis para desenvolvedores estão em doc/developer-notes.md.  

---

### Testes  
*Testes e revisão de código* são o principal gargalo no desenvolvimento, já que recebemos mais pull requests do que conseguimos revisar e testar rapidamente. Seja paciente e colabore testando as alterações de outros desenvolvedores. Este é um projeto crítico para segurança, onde qualquer erro pode custar muito dinheiro aos usuários.  

#### Testes Automatizados  
- Recomenda-se que desenvolvedores escrevam testes unitários para novos códigos e adicionem testes para códigos antigos.  
- Os testes podem ser compilados e executados com: ctest (assumindo que não foram desativados durante a configuração do sistema de compilação).  
- Mais detalhes sobre testes estão em /src/test/README.md.  
- Há também testes de regressão e integração, escritos em Python, que podem ser executados com: build/test/functional/test_runner.py (assumindo que build é o diretório de compilação).  
- Sistemas de Integração Contínua (CI) garantem que todas as pull requests sejam compiladas e testadas automaticamente para Windows, Linux e macOS.

### Garantia de Qualidade (QA) Manual 
- Mudanças devem ser testadas por alguém diferente do desenvolvedor que escreveu o código.  
- Isso é especialmente importante para mudanças grandes ou de alto risco.  
- Adicionar um plano de teste à descrição da pull request pode ser útil quando o teste das mudanças não for simples.  

---

### Traduções  
- Alterações e novas traduções podem ser enviadas pela página do Transifex do Bitcoin Core.  
- As traduções são periodicamente extraídas do Transifex e mescladas no repositório git.  
- Importante: Pull requests relacionadas a traduções não são aceitas no GitHub, pois seriam sobrescritas na próxima sincronização com o Transifex.  

Consulte o processo de tradução para mais detalhes sobre como isso funciona.
