# PS2 H.264 Player

Player H.264 para PlayStation 2 construído a partir do decoder
[h264bsd](https://github.com/oneam/h264bsd) e de uma camada fina de integração
para o EE/GS. O projeto inclui um fluxo de trabalho completo para embutir um
bitstream H.264 no ELF e gerar uma imagem ISO executável no console/emulador.

## Estrutura do projeto

- `src/`
  - `main.c`: laço principal, feed do decoder e apresentação no GS.
  - `ps2_decoder.c`: wrapper sobre o h264bsd com gerenciamento de frames/DPB.
  - `ps2_video.c`: inicialização do GSKit, alocação/transferência de texturas e
    apresentação.
- `include/`: cabeçalhos da camada PS2 e o blob `raw_video.h` gerado.
- `third_party/`: bibliotecas vendorizadas (`h264bsd`, `edge264`, `tinyh264`).
- `build_payload.py`: utilitário simples para copiar um bitstream externo.
- `disc/SYSTEM.CNF`: configuração de boot para gerar a ISO.

## Pré-requisitos

- Toolchain [ps2dev](https://ps2dev.github.io/ps2toolchain/) instalado e nas variáveis
  de ambiente:

  ```sh
  export PS2DEV=/usr/local/ps2dev
  export PS2SDK=$PS2DEV/ps2sdk
  export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin
  ```

- `ffmpeg` (opcional) para transcodar vídeos para o formato aceito.
- `mkisofs` para gerar a imagem ISO.

## Atualizando o vídeo embutido

1. Converta o vídeo de entrada para um elementary stream H.264 (Annex B). Exemplo:

   ```sh
   ffmpeg -i input.mp4 \
          -vf "scale=320:240:flags=lanczos,fps=24" \
          -c:v libx264 -profile:v baseline -level 3.0 -pix_fmt yuv420p \
          -x264opts keyint=24:min-keyint=24:scenecut=0 \
          -an -f h264 raw_video.h264
   ```

2. Gere o header `include/raw_video.h`:

   ```sh
   python - <<'PY'
   from pathlib import Path
   data = Path('raw_video.h264').read_bytes()
   with open('include/raw_video.h', 'w') as f:
       f.write('#ifndef RAW_VIDEO_H\n#define RAW_VIDEO_H\n\n')
       f.write(f'static const unsigned int g_video_size = {len(data)};\n')
       f.write('static const unsigned char g_video_data[] = {\n')
       for i, b in enumerate(data):
           if i % 12 == 0:
               f.write('    ')
           f.write(f'0x{b:02X}')
           if i != len(data) - 1:
               f.write(', ')
           if (i + 1) % 12 == 0:
               f.write('\n')
       if len(data) % 12:
           f.write('\n')
       f.write('};\n\n#endif /* RAW_VIDEO_H */\n')
   PY
   ```

## Compilação

```sh
make clean && make
```

Saída principal:

- `h264_ps2_player.elf`: executável EE.

Para gerar uma ISO simples contendo o ELF:

```sh
mkisofs -o h264_ps2_player.iso \
        -V H264PS2 -sysid PLAYSTATION -iso-level 1 \
        -graft-points /SYSTEM.CNF=disc/SYSTEM.CNF /H264PS2.ELF=h264_ps2_player.elf
```

## Execução

- Em emuladores como PCSX2: selecione a ISO ou carregue o ELF via `host:`.
- Em hardware real: grave a ISO em mídia/USB e utilize uLaunchELF/OSD.

Durante a execução o player registra mensagens no TTY (`scr_printf`) indicando
estado do decoder, resolução detectada e contagem de frames apresentados.

## Próximos passos

- Otimizações de pipeline: uso de VUs, DMA e apresentação em YUV para reduzir
  cópias.
- Streaming a partir de mídia (`mass:`/CDVD) em vez de bitstream embutido.
- Suporte a entrada de controle (pausa/troca de vídeo) e reprodução de áudio.
- Layout de build multiplataforma (scripts para Windows/macOS + Docker).

O material leg Legacy anterior permanece em `old/` somente para consulta.
