# DecoderH264 (nova base)

Esta versão reiniciada utiliza o decodificador [h264bsd](https://github.com/oneam/h264bsd)
como backend para oferecer um player H.264 rodando no PlayStation 2.

## Estrutura

- `src/` contém o código específico do PS2 (`main.c`, `ps2_decoder.c`, `ps2_video.c`).
- `include/` expõe os headers da camada PS2.
- `third_party/h264bsd/` mantém o código-fonte do decoder open source.
- `Makefile` monta o ELF `h264_ps2_player.elf` usando o toolchain PS2DEV.

## Build

1. Garanta que as variáveis de ambiente estejam configuradas:
   ```sh
   export PS2DEV=/usr/local/ps2dev
   export PS2SDK=$PS2DEV/ps2sdk
   export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin
   ```
2. Rode `make` na raiz do projeto.
3. O resultado (`h264_ps2_player.elf`) pode ser carregado via uLaunchELF, ps2link, etc.

## Uso

- Copie um arquivo `VIDEO.264` (H.264 baseline/AVC) para `host:` (ps2link), `mass:` (USB)
  ou para o mesmo diretório do ELF.
- Ao executar, o player tenta carregar o bitstream em memória, decodificar e exibir os
  frames em tempo real na GS via textura RGBA.

## Pendências / Melhorias

- Otimizar upload das texturas e pipeline de renderização (DMA, VU).
- Fazer streaming por chunks ao invés de carregar todo o arquivo de uma vez.
- Adicionar controles (pausa, troca de vídeo) via gamepad.
- Tratar áudio e sincronização AV.

O código anterior permanece arquivado em `old/` para referência histórica.
