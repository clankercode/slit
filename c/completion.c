#include "completion.h"
#include <stdio.h>
#include <string.h>

static const char bash_completion[] =
"_slit_completions() {\n"
"    local cur=${COMP_WORDS[COMP_CWORD]}\n"
"    local opts='-n --lines --max-lines -o --output -a --append --tee-format\n"
"        -l --line-numbers --color -w --wrap -t --timestamp --truncation-char\n"
"        --layout --box --rounded --compact --minimal --none --quote --quote-bg\n"
"        --spinner -d --debug --log-file --help --version'\n"
"    case \"$cur\" in\n"
"        --color=*) COMPREPLY=($(compgen -W 'auto always never' -- \"${cur#--color=}\"));;\n"
"        --layout=*) COMPREPLY=($(compgen -W 'box rounded compact minimal none quote' -- \"${cur#--layout=}\"));;\n"
"        --spinner=*) COMPREPLY=($(compgen -W 'braille dots arrows off' -- \"${cur#--spinner=}\"));;\n"
"        --tee-format=*) COMPREPLY=($(compgen -W 'raw display' -- \"${cur#--tee-format=}\"));;\n"
"        -o|--output|--log-file) COMPREPLY=($(compgen -f -- \"$cur\"));;\n"
"        --*) COMPREPLY=($(compgen -W \"$opts\" -- \"$cur\"));;\n"
"        -*) COMPREPLY=($(compgen -W \"$opts\" -- \"$cur\"));;\n"
"    esac\n"
"}\n"
"complete -F _slit_completions slit\n";

static const char zsh_completion[] =
"#compdef slit\n"
"_slit() {\n"
"    _arguments {\n"
"        '-n[Number of lines]:lines' \\\n"
"        '--max-lines[Max lines to buffer]:max' \\\n"
"        '-o[Output file]:file:_files' \\\n"
"        '-a[Append to output]' \\\n"
"        '--tee-format[Format]:format:(raw display)' \\\n"
"        '-l[Show line numbers]' \\\n"
"        '--color[Color mode]:mode:(auto always never)' \\\n"
"        '-w[Wrap lines]' \\\n"
"        '-t[Show timestamps]' \\\n"
"        '--truncation-char[Truncation char]:char' \\\n"
"        '--layout[Layout]:style:(box rounded compact minimal none quote)' \\\n"
"        '--box[Box layout]' \\\n"
"        '--rounded[Rounded layout]' \\\n"
"        '--compact[Compact layout]' \\\n"
"        '--minimal[Minimal layout]' \\\n"
"        '--none[No layout]' \\\n"
"        '--quote[Quote layout]' \\\n"
"        '--quote-bg[Quote bg color]:color' \\\n"
"        '--spinner[Spinner]:style:(braille dots arrows off)' \\\n"
"        '-d[Debug mode]' \\\n"
"        '--log-file[Log file]:file:_files' \\\n"
"        '--help[Show help]' \\\n"
"        '--version[Show version]' \\\n"
"    }\n"
"}\n"
"_slit \"$@\"\n";

static const char fish_completion[] =
"complete -c slit -s n -l lines -d 'Number of lines to display' -x\n"
"complete -c slit -l max-lines -d 'Maximum lines to buffer' -x\n"
"complete -c slit -s o -l output -d 'Output file' -r\n"
"complete -c slit -s a -l append -d 'Append to output'\n"
"complete -c slit -l tee-format -d 'Tee format' -xa 'raw display'\n"
"complete -c slit -s l -l line-numbers -d 'Show line numbers'\n"
"complete -c slit -l color -d 'Color mode' -xa 'auto always never'\n"
"complete -c slit -s w -l wrap -d 'Wrap long lines'\n"
"complete -c slit -s t -l timestamp -d 'Show timestamps'\n"
"complete -c slit -l truncation-char -d 'Truncation character'\n"
"complete -c slit -l layout -d 'Layout style' -xa 'box rounded compact minimal none quote'\n"
"complete -c slit -l box -d 'Box layout'\n"
"complete -c slit -l rounded -d 'Rounded layout'\n"
"complete -c slit -l compact -d 'Compact layout'\n"
"complete -c slit -l minimal -d 'Minimal layout'\n"
"complete -c slit -l none -d 'No layout'\n"
"complete -c slit -l quote -d 'Quote layout'\n"
"complete -c slit -l quote-bg -d 'Quote background color'\n"
"complete -c slit -l spinner -d 'Spinner style' -xa 'braille dots arrows off'\n"
"complete -c slit -s d -l debug -d 'Debug mode'\n"
"complete -c slit -l log-file -d 'Log file' -r\n"
"complete -c slit -l help -d 'Show help'\n"
"complete -c slit -l version -d 'Show version'\n";

void completion_print(const char *shell) {
    if (!shell) {
        fprintf(stderr, "Usage: slit completion <shell>\nSupported: bash, zsh, fish\n");
        return;
    }
    if (strcmp(shell, "bash") == 0) {
        fwrite(bash_completion, 1, sizeof(bash_completion) - 1, stdout);
    } else if (strcmp(shell, "zsh") == 0) {
        fwrite(zsh_completion, 1, sizeof(zsh_completion) - 1, stdout);
    } else if (strcmp(shell, "fish") == 0) {
        fwrite(fish_completion, 1, sizeof(fish_completion) - 1, stdout);
    } else {
        fprintf(stderr, "Unsupported shell: %s\nSupported: bash, zsh, fish\n", shell);
    }
}
