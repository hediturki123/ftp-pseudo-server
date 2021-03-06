#include "csapp.h"
#include <dirent.h>
#define _GNU_SOURCE
#define TAILLE_BUFFER 10


void nom_fichier(char *buf, char *nom){
    int i;

    for(i = 0; buf[i] != '\n' && buf[i] != '\0'; i++){
        nom[i] = buf[i];
    }

    nom[i] = '\0';
}

void lecture_fichier(char buf[], int connfd){
    int fd,n;
    char Rbuf[MAXLINE],nom[MAXLINE],taille[4];
    rio_t fich;
    nom_fichier(buf, nom);
        
    fd = open(nom, O_RDONLY);
    
    if(fd < 0){
        strcpy(Rbuf,"Erreur lors de l'ouverture du fichier\n");
        Rio_writen(connfd, Rbuf, strlen(Rbuf));
    }
    else{ 
        Rio_readinitb(&fich,fd);
        while((n = Rio_readnb(&fich, Rbuf, TAILLE_BUFFER)) > 0){
            sprintf(taille, "%d", n);
            Rio_writen(connfd, taille, 4);
            Rio_writen(connfd, Rbuf, n); 
            memset(Rbuf, 0, sizeof(Rbuf));
            memset(taille, 0, sizeof(taille));
        }

        Rio_writen(connfd, "0", 4);
    }
    Close(fd);
}

int transfert_fichier(char fichier[], int connfd){
    int fd;
    size_t n;
    char buf[MAXLINE];
    char message[MAXLINE];
    char taille[4];
    char taille_paquets[100];
    int nbre_de_paquets = 0;
    rio_t rio;
    fd = open(fichier, O_RDONLY, 0);
    
    if(fd < 0){
        strcpy(message, "Erreur de fichier\n");
        Rio_writen(connfd, message, strlen(message));
    
    } else {

        strcat(fichier, "\n");
        Rio_writen(connfd, fichier, strlen(fichier));
        Rio_readinitb(&rio, fd);

        while((n = Rio_readnb(&rio, buf, TAILLE_BUFFER)) > 0){
            sprintf(taille, "%ld", n);
            Rio_writen(connfd, taille, 4);
            Rio_writen(connfd, buf, n); 
            memset(buf, 0, sizeof(buf));
            memset(taille, 0, sizeof(taille));
            nbre_de_paquets++;
        }

        Rio_writen(connfd, "0", 4);
        sprintf(taille_paquets, "%d", nbre_de_paquets);
        strcat(taille_paquets, "\n");
        Rio_writen(connfd, taille_paquets,strlen(taille_paquets));
        memset(buf, 0, TAILLE_BUFFER);
        Close(fd);
    }
    return nbre_de_paquets;
}


void decoupe(char commande[], char fichier[], char buf[]){
    int sauve_i, espace = 0;

    for(int i = 0; buf[i] != '\0' && buf[i] != '\n'; i++){
        
        if(buf[i]==' '){
            
            if(buf[i+1] == '-' && buf[i+2] == 'r'){
                espace = 1;
                sauve_i = i + 4;
                strcat(commande, " -r");
            
            } else {
                espace = 1;
                sauve_i = i+1;
            }

        } else {
            switch (espace){
                
                case 0:
                    commande[i] = buf[i];
                    break;
                
                case 1:
                    fichier[i-sauve_i] = buf[i];
                    break;
            }
        }
    }
}


void crash_et_reprise(int connfd, rio_t crio){
    int fd;
    size_t n;
    char buf[MAXBUF];
    char message[MAXLINE];
    char taille[4];
    char nb_paquets[MAXLINE];
    char fichier[MAXBUF];
    int paquet;
    rio_t rio;
    struct stat stat;
    
    Rio_readlineb(&crio, buf, MAXBUF);
    
    decoupe(fichier, nb_paquets, buf);
    paquet = atoi(nb_paquets);
    strcat(fichier, "\n");
    Rio_writen(connfd, fichier, MAXBUF);

    fichier[strlen(fichier)-1] = '\0';
    
    fd = open(fichier, O_RDONLY, 0);    
    
    
    if(fd < 0){
        strcpy(message, "Erreur de fichier\n");
        Rio_writen(connfd, message, strlen(message));
    
    } else {

        Rio_readinitb(&rio, fd);
        Fstat (fd, &stat);
        int nbre_p = stat.st_size/TAILLE_BUFFER;
        sprintf(buf, "%d", nbre_p);
        // on recupere le nombre de paquets du fichier        
        
        memset(buf, 0, strlen(buf));
        if (paquet != (nbre_p/TAILLE_BUFFER)){
            Lseek(fd,paquet*TAILLE_BUFFER,SEEK_SET);
        }

        while((n = Rio_readnb(&rio, buf, TAILLE_BUFFER)) > 0){
            
            sprintf(taille, "%ld", n);
            Rio_writen(connfd, taille, 4);
            Rio_writen(connfd, buf, n); 
        }
        Rio_writen(connfd, "0", 4);
        memset(buf, 0, TAILLE_BUFFER);
        Close(fd);
    }

}


void recup_fichier(char fichier[],int connfd,rio_t rio){
    int fd, n;
    char buf[MAXBUF];
    char message[MAXBUF];
    nom_fichier(fichier, buf);
    fd = open(buf, O_CREAT | O_WRONLY, 0666);

    if(fd < 0){
        strcpy(message, "Erreur de fichier\n");
        Rio_writen(connfd, message, strlen(message));
    
    } else {
        strcpy(message, "Création du fichier ok\n");
        Rio_writen(connfd, message, strlen(message));
        
        while((n = Rio_readnb(&rio,buf,TAILLE_BUFFER))>0){
            write(fd, buf, n);
        }
    }
    close(fd);
}

void affiche_rep(int connfd, char fichier[MAXBUF]){ 
    
    struct dirent *dir;
    char nom[MAXBUF];
    DIR *d;
    if(fichier[0]=='-'){
        strcpy(nom,"Commande inconnu\n");
        Rio_writen(connfd,nom,strlen(nom));
    }
    else{
        if(strlen(fichier)!=0){
            if((d = opendir(fichier))==NULL){
                strcpy(nom,"Répertoire inéxistant\n");
                Rio_writen(connfd,nom,strlen(nom));
            }
        }
        else{d = opendir(".");}
        if (d){
            while ((dir = readdir(d)) != NULL){
                strcpy(nom,dir->d_name);
                if(nom[0]!='.'){
                    strcat(nom, " ");
                    Rio_writen(connfd,nom,strlen(nom));
                }
            }
            strcpy(nom, "\n");
            Rio_writen(connfd,nom,strlen(nom));
            closedir(d);
        }
    }
}

void creation_repertoire(char fichier[], int connfd){
    char message[MAXBUF];
    fichier[strlen(fichier)] = '\0';

    if(chdir(fichier) == 0){
        strcpy(message,"Répertoire existant !\n");
        chdir("..");
    
    } else {

        if(mkdir(fichier,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0){
            strcpy(message,"Erreur lors de la création du repertoire\n");
        
        } else {
            strcpy(message,"Repertoire créé\n");
        }
    }
    
    Rio_writen(connfd,message,strlen(message));
}

void remove_file(char fichier[], int connfd){
    char message[MAXBUF];
    fichier[strlen(fichier)] = '\0';
    
    if(remove(fichier)==0){
        strcpy(message,"Fichier supprimé\n");
    
    } else {
        strcpy(message,"Erreur\n");
    }

    Rio_writen(connfd,message,strlen(message));
}

void change_directory(int connfd, char fichier[MAXBUF]){
    char nom[MAXBUF];
    char message[MAXBUF];
    nom_fichier(fichier,nom);

    if (chdir(nom) != 0){
        strcpy(message,"Aucun fichier dans ce repertoire\n");
    
    } else {
        strcpy(message,"ok\n");
    }

    Rio_writen(connfd,message,strlen(message));
}

int remove_rec(char fichier[], int connfd){
    int n = 0;
    struct dirent *dir;
    char buf[MAXBUF];
    DIR *d = opendir(fichier);
    chdir(fichier);
    
    if (d){
        while ((dir = readdir(d)) != NULL){
            
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")){}
            
            else {
                if (dir->d_type == DT_DIR){
                    n += remove_rec(dir->d_name, connfd);
                
                } else {
                    strcpy(buf, dir->d_name);
                    if(remove(buf) != 0){n++;}
                }
            }
        }

        chdir("..");
        closedir(d);
        rmdir(fichier);
    }
    return n;
}

void remove_folder(char fichier[],int connfd){
    int marque = 0,nb_de_sous_dossier = 0, i, j;
    char message[MAXBUF], dir[MAXBUF];
    fichier[strlen(fichier)] = '\0';
    
    for(i = 0; fichier[i] != '\0'; i++){
        if(fichier[i] == '/'){
            nb_de_sous_dossier++;
        }
    }
    
    if(nb_de_sous_dossier != 0){
        
        for(i = strlen(fichier); fichier[i] != '/' ;i--){}

        fichier[i] = '$';
        for(i = 0; fichier[i] != '\0'; i++){
            
            if(fichier[i] == '$'){
                marque = 1;
                i++;
                j = 0;
            }
            if(marque){
                fichier[j] = fichier[i];
                j++;
            
            } else {
                dir[i] = fichier[i];
            }
        }
        fichier[j] = '\0';
        dir[strlen(dir)] = '\0';
        chdir(dir);
    }
    
    if(remove_rec(fichier, connfd) == 0){
        strcpy(message, "Le répertoire a été supprimé\n");
        
        for(i = 0; i < nb_de_sous_dossier; i++){
            chdir("..");
        }
    } else {
        strcpy(message, "Le répertoire n'a pas pu être supprimé\n");
    }
    Rio_writen(connfd, message, strlen(message));
}

void chemin(int connfd){
    char cwd[MAXBUF];
    getcwd(cwd, sizeof(cwd));
    cwd[strlen(cwd)] = '\n';
    Rio_writen(connfd, cwd, strlen(cwd));
}


void demande_client(int connfd){
    size_t n;
    char buf[MAXLINE];
    char commande[10];
    char fichier[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while(1){
        if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
            decoupe(commande,fichier,buf);
            if(!strcmp(commande, "get")){
                transfert_fichier(fichier,connfd);
            }

            else if(!strcmp(commande, "resume")){
                crash_et_reprise(connfd,rio);
            }

            else if(!strcmp(commande, "cat")){
                lecture_fichier(fichier,connfd);
            }
            
            else if(!strcmp(commande, "ls")){
                affiche_rep(connfd, fichier);
            }

            else if(!strcmp(commande, "put")){
                recup_fichier(fichier, connfd, rio);
            }

            else if(!strcmp(commande, "mkdir")){
                creation_repertoire(fichier, connfd);
            }

            else if(!strcmp(commande, "cd")){
                change_directory(connfd, fichier);
            }

            else if(!strcmp(commande, "rm")){
                remove_file(fichier, connfd);
            }

            else if(!strcmp(commande, "rm -r")){
                remove_folder(fichier, connfd);
            }

            else if(!strcmp(commande, "pwd")){
                chemin(connfd);
            }

            else if(!strcmp(commande,"touch")){
                int fd;
                char message[MAXBUF];
                fichier[strlen(fichier)]='\0';
                if((fd=open(fichier,O_CREAT,0666))<0){
                    strcpy(message,"Erreur lors de la création du fichier\n");
                    Rio_writen(connfd,message,MAXBUF);
                }
                else{
                    strcpy(message,"Fichier créé\n");
                    Rio_writen(connfd,message,MAXBUF);
                }
                Close(fd);
            }

            else if(!strcmp(commande, "bye")){
                return;
            }
        }
    memset(buf, 0, MAXLINE);
    memset(fichier, 0, MAXLINE);
    memset(commande, 0, 10);
    }
}
