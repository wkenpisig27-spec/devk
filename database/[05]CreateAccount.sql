USE AccountServer;
INSERT INTO account_login (name, password) VALUES ('admin', '$2a$12$Mdgo33/gPNo8Ob4yHaKmKORaILwezVJrKyQqZqobLh3m7j14nHT1i');
USE GameDB;
INSERT INTO account (act_id, act_name, gm) VALUES ((SELECT ISNULL(MAX(act_id) + 1,1) FROM account), 'admin', 99);
