/*
 * (C) 2003-2025 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 * Module ns_login created by k4be and reworked by reverse to make it work on 2.1
 * Include the following in your nickserv.conf
 *
 * module { name = "ns_login" }
 * command { service = "NickServ"; name = "LOGIN"; command = "nickserv/login"; }
 *
 */

 #include "module.h"
 #include "modules/ns_cert.h"
 
 static ServiceReference<NickServService> nickserv("NickServService", "NickServ");
 
 typedef std::map<Anope::string, ChannelStatus> NSLoginInfo;
 
 class NSLoginSvsnick
 {
 public:
     Reference<User> from;
     Anope::string to;
 };
 
 class NSLoginRequest final : public IdentifyRequest
 {
     CommandSource source;
     Command *cmd;
     Anope::string user;
 
 public:
     NSLoginRequest(Module *o, CommandSource &src, Command *c, const Anope::string &nick, const Anope::string &pass)
         : IdentifyRequest(o, nick, pass, src.GetUser() ? src.GetUser()->ip.addr() : ""), source(src), cmd(c), user(nick) { }
 
     void OnSuccess() override
     {
         User *u = User::Find(user, true);
         if (!source.GetUser() || !source.service)
             return;
 
         NickAlias *na = NickAlias::Find(user);
         if (!na)
             return;
 
         Log(LOG_COMMAND, source, cmd) << "for " << na->nick;
 
         /* Nick is being held by us, release it */
         if (na->HasExt("HELD"))
         {
             nickserv->Release(na);
             source.Reply(_("Service's hold on \002%s\002 has been released."), na->nick.c_str());
         }
         if (!u)
         {
             if (IRCD->CanSVSNick)
                 IRCD->SendForceNickChange(source.GetUser(), GetAccount(), Anope::CurTime);
             source.GetUser()->Identify(NickAlias::Find(user)); // Fix: Ensure correct type
             Log(LOG_COMMAND, source, cmd) << "and identified to " << na->nick << " (" << na->nc->display << ")";
             source.Reply(_("Password accepted - you are now recognized."));
         }
         else if (u->Account() == na->nc)
         {
             if (!source.GetAccount() && na->nc->HasExt("NS_SECURE"))
             {
                 source.GetUser()->Identify(NickAlias::Find(user)); // Fix: Ensure correct type
                 Log(LOG_COMMAND, source, cmd) << "and was automatically identified to " << u->Account()->display;
             }
 
             if (Config->GetModule("ns_login").Get<bool>("restoreonrecover"))
             {
                 if (!u->chans.empty())
                 {
                     NSLoginInfo *ei = source.GetUser()->Extend<NSLoginInfo>("login");
                     for (auto &it : u->chans)
                         (*ei)[it.first->name] = it.second->status;
                 }
             }
 
             u->SendMessage(source.service, _("This nickname has been recovered by %s. If you did not do\n"
                                              "this then %s may have your password, and you should change it."),
                            source.GetNick().c_str(), source.GetNick().c_str());
 
             Anope::string buf = source.command.upper() + " command used by " + source.GetNick();
             u->Kill(*source.service, buf);
 
             source.Reply(_("Ghost with your nick has been killed."));
 
             if (IRCD->CanSVSNick)
                 IRCD->SendForceNickChange(source.GetUser(), GetAccount(), Anope::CurTime);
         }
         else
         {
             if (!source.GetAccount() && na->nc->HasExt("NS_SECURE"))
             {
                 source.GetUser()->Identify(NickAlias::Find(na->nick)); // Fix: Ensure correct type
                 Log(LOG_COMMAND, source, cmd) << "and was automatically identified to " << na->nick << " (" << na->nc->display << ")";
                 source.Reply(_("You have been logged in as \002%s\002."), na->nc->display.c_str());
             }
 
             u->SendMessage(source.service, _("This nickname has been recovered by %s."), source.GetNick().c_str());
 
             if (IRCD->CanSVSNick)
             {
                 NSLoginSvsnick *svs = u->Extend<NSLoginSvsnick>("login_svsnick");
                 svs->from = source.GetUser();
                 svs->to = u->nick;
             }
 
             if (nickserv)
                 nickserv->Collide(u, na);
 
             if (IRCD->CanSVSNick)
             {
                 if (nickserv)
                     nickserv->Release(na);
 
                 source.Reply(_("You have regained control of \002%s\002."), u->nick.c_str());
             }
             else
             {
                 source.Reply(_("The user with your nick has been removed. Use this command again\n"
                                "to release services's hold on your nick."));
             }
         }
     }
 
     void OnFail() override
     {
         if (NickAlias::Find(GetAccount()) != NULL)
         {
             source.Reply(ACCESS_DENIED);
             if (!GetPassword().empty())
             {
                 Log(LOG_COMMAND, source, cmd) << "with an invalid password for " << GetAccount();
                 if (source.GetUser())
                     source.GetUser()->BadPassword();
             }
         }
         else
             source.Reply(NICK_X_NOT_REGISTERED, GetAccount().c_str());
     }
 };
 
 class CommandNSLogin final : public Command
 {
 public:
     CommandNSLogin(Module *creator) : Command(creator, "nickserv/login", 1, 2)
     {
         this->SetDesc(_("Identifies you to services and regains control of your nick"));
         this->SetSyntax(_("\037nickname\037 [\037password\037]"));
         this->AllowUnregistered(true);
     }
 
     void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
     {
         const Anope::string &nick = params[0];
         const Anope::string &pass = params.size() > 1 ? params[1] : "";
 
         User *user = User::Find(nick, true);
 
         if (user && source.GetUser() == user)
         {
             source.Reply(_("You can't %s yourself!"), source.command.lower().c_str());
             return;
         }
 
         const NickAlias *na = NickAlias::Find(nick);
 
         if (!na)
         {
             source.Reply(NICK_X_NOT_REGISTERED, nick.c_str());
             return;
         }
         else if (na->nc->HasExt("NS_SUSPENDED"))
         {
             source.Reply(NICK_X_SUSPENDED, na->nick.c_str());
             return;
         }
 
         bool ok = (source.GetAccount() == na->nc);
 
         NSCertList *cl = na->nc->GetExt<NSCertList>("certificates");
         if (source.GetUser() && !source.GetUser()->fingerprint.empty() && cl && cl->FindCert(source.GetUser()->fingerprint))
             ok = true;
 
         if (!ok && !pass.empty())
         {
             NSLoginRequest *req = new NSLoginRequest(owner, source, this, na->nick, pass);
             FOREACH_MOD(OnCheckAuthentication, (source.GetUser(), req));
             req->Dispatch();
         }
         else
         {
             NSLoginRequest req(owner, source, this, na->nick, pass);
             if (ok)
                 req.OnSuccess();
             else
                 req.OnFail();
         }
     }
 };
 
 class NSLogin final : public Module
 {
     CommandNSLogin commandnslogin;
 
 public:
     NSLogin(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR),
         commandnslogin(this)
     {
         if (Config->GetModule("nickserv").Get<bool>("nonicknameownership"))
             throw ModuleException(modname + " cannot be used with options:nonicknameownership enabled");
     }
 };
 
 MODULE_INIT(NSLogin)
 
