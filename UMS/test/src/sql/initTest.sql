INSERT INTO vishnu("updatefreq", "formatiduser", "formatidjob", "formatidfiletransfer", "formatidmachine", "usercpt", "jobcpt", "filesubcpt", "machinecpt") values('100', 'user_$CPT', 'job_$CPT', 'transfer_$CPT', 'machine_$CPT', '2', '2', '2', '2');
INSERT INTO users("userid", "pwd", "firstname", "lastname", "privilege", "email", "passwordstate", status, "vishnu_vishnuid") values('admin_1', 'admin', 'jean', 'dupont', '1', 'admin@admin.com', '1', '1', '1');
INSERT INTO users("userid", "pwd", "firstname", "lastname", "privilege", "email", "passwordstate", status, "vishnu_vishnuid") values('user_1', 'user', 'jeannot', 'dupont', '0', 'user@user.com', '1', '1', '1');
INSERT INTO machine("vishnu_vishnuid", "name", "site", "machineid")values('1', 'tiger', 'berlin', 'machine_1');

