#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>

#define PROMPT  ">>> "

struct command {
    const char * const cmd;     //command string name
    int (* handler)(void);      //function pointer
    const char * const help;    //command string help text
};

int handle_blink(void)
{
    printf("Hello from %s\n", __FUNCTION__);
    return 0;
}

int handle_shutdown(void)
{
    printf("Hello from %s\n", __FUNCTION__);
    return 0;
}

int handle_reboot(void)
{
    printf("Hello from %s\n", __FUNCTION__);
    return 0;
}

int handle_info(voi)
{
    printf("Firmware build %s, %s\n", __DATE__, __TIME__);
    return 0;
}

int handle_help(void);
struct command g_cmds[] = 
{
    { "help",       handle_help, "Prints this help message" },
    { "info",       handle_info, "Prints info about the build" },
    { "blink",      handle_blink , "Blinks an LED!" },
    { "shutdown",   handle_shutdown , "System shutdown" },
    { "reboot",     handle_reboot, "System reboot" }
};

int handle_help(void)
{
    int i;
    printf("Available commands: \n");
    
    for(i=0; i<sizeof(g_cmds)/sizeof(struct command); i++){
        printf("\t%s: %s\n", g_cmds[i].cmd, g_cmds[i].help);
    }

    return 0;
}


/*
 * Match the contents of 'buf' with the global set of
 * supported commands. This function returns 0 on success
 * with 'match' populated with the address of the matched
 * command structure. Otherwise it returns -1 and the
 * contents of 'match' is undefined.
 *
 * @param buf       A buffer containing the string to match
 * @param match     An out-parameter that will point to the matched command
 * @return 0 on success, -1 otherwise
 */
int find_match(char *buf, struct command **match)
{
    int i;
    int l1,l2;
    int min_len;
    struct command *local_match = NULL;
    int num_matches = 0;

    for(i=0; i<sizeof(g_cmds)/sizeof(struct command); i++){
        l1 = strlen(g_cmds[i].cmd);
        l2 = strlen(buf);
        min_len = (l2 < l1) ? (l2) : (l1);
        
        /*
         * We're only comparing the min length here, so
         * we'll get an exact match (return == 0) if buf
         * equals a string or is a subset of a string.
         */
        if(0 == memcmp(g_cmds[i].cmd, buf, min_len)){
            local_match = &g_cmds[i];
            num_matches++;
        }
    }
    
    /* If we found exactly one match, return it. Otherwise we found none or multiple (e.g. non-exact match) */
    if(1 == num_matches){
        *match = local_match; 
        return 0;
    }

    return -1;
}

/* Make the terminal behave a bit more like a a serial connection */
void set_term_behavior(void)
{
    struct termios ctrl;
    tcgetattr(0, &ctrl);
    ctrl.c_lflag &= ~(ICANON | ECHO); // turning off canonical mode makes input unbuffered
    tcsetattr(0, TCSANOW, &ctrl);
    
    tcgetattr(1, &ctrl);
    ctrl.c_lflag &= ~ICANON;
    tcsetattr(1, TCSANOW, &ctrl);
}

int main(int argc, char **argv)
{
    char input[512] = {0}; //zero the array, this automatically 'null terminates' the array for us to start
    int index;
    char c;
    struct command *cmd = 0;

    set_term_behavior();
    printf("Welcome to simple autocomplete\n\n");

    /* Loop forever receiving commands */
    while(1){
        memset(input, 0, sizeof(input));    //clear out the old contents of the input buffer
        index = 0;
        cmd = NULL;

        printf("\n%s", PROMPT);
    
        /* Get characters from stdin until the user hits RETURN or we find an autocomplete match */
        do{
            /* Get the next character, but make sure it'll fit (with room for NULL to terminate the string */
            if(index < (sizeof(input) - 1)){
                c = getchar();
    
                /* Add/echo character to buffer only if printable */
                if( (c >= 0x20) && (c <= 0x7e) ){
                    input[index++] = c;//tolower(c); 
                    putchar(c); //Echo what the user input
                }
   
            }else{
                //error case, input too long -- break
                printf("Ooops, input too long!\n");
                break;
            }

            /* Check for autocomplete match if user hits TAB */
            switch(c){
                case 0x7f:
                    if(0 == index){
                        break;  //Nothing to delete
                    }

                    /* Clear the row and rewrite the buffer */
                    putchar('\r');
                    for(int i=0; i<index + sizeof(PROMPT); i++){
                        putchar(' ');
                    }

                    input[--index] = '\0';
                    printf("\r%s%s", PROMPT, input);
                    break;

                case '\t':
                    if(0 == find_match(input, &cmd)){
                        if(index == strlen(cmd->cmd)){
                            break;
                        }

                        printf("%s", cmd->cmd + index); //echo the rest of the command
                        memcpy(input + index, cmd->cmd + index, strlen(cmd->cmd) - index);
                        index = strlen(cmd->cmd);
                    }
                    break;
            }

        }while(c != '\n');
        putchar(c);
 
        /* This pointer is NULL if the user didn't hit TAB, so attempt to find a match */
        if(NULL == cmd){
            find_match(input, &cmd);
        }
        
        /* cmd != NULL if the user hit TAB and we found a match earlier or in the preceeding line */
        if(NULL != cmd){
            cmd->handler();
        }else{
            printf("Unknown command '%s'\n", input);
        }
    } 

    return 0;
}


