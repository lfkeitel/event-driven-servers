#!/usr/bin/env perl
#
# mavis_tacplus_radius.pl
# (C)2001-2011 Marc Huber <Marc.Huber@web.de>
# All rights reserved.
#
# $Id: mavis_tacplus_radius.pl,v 1.21 2016/07/13 17:38:36 marc Exp marc $
#
# radius passwd backend for libmavis_external.so
#

# Test input:
# 0 TACPLUS
# 4 $USER
# 8 $PASS
# 49 AUTH
# =

my $RADIUS_HOST = 'localhost';
my $RADIUS_SECRET = 'secret';
my $RADIUS_GROUP_ATTR = undef;
my $RADIUS_DICTIONARY = undef;

my ($ACCESS_REQUEST, $ACCESS_ACCEPT);

if (eval("require Authen::Radius")) {
	import Authen::Radius;
	$ACCESS_REQUEST = 1; $ACCESS_ACCEPT = 2;
} elsif (eval("require Authen::Simple::RADIUS")) {
	import Authen::Simple::RADIUS;
} else {
	die "Error: Neither\n\tAuthen::Radius nor\n\tAuthen::Simple::RADIUS\nare available on your system.";
}

print STDERR "Using ", $ACCESS_REQUEST ? "Authen::Radius" : "Authen::Simple::RADIUS", " Perl module.\n";

$RADIUS_HOST = $ENV{'RADIUS_HOST'} if exists $ENV{'RADIUS_HOST'};
$RADIUS_SECRET = $ENV{'RADIUS_SECRET'} if exists $ENV{'RADIUS_SECRET'};
$RADIUS_DICTIONARY = $ENV{'RADIUS_DICTIONARY'} if exists $ENV{'RADIUS_DICTIONARY'};

if ($ACCESS_REQUEST) {
	$RADIUS_GROUP_ATTR = $ENV{'RADIUS_GROUP_ATTR'} if exists $ENV{'RADIUS_GROUP_ATTR'};
}

my @RADIUS_NODELIST = split(',', $RADIUS_HOST);
$RADIUS_HOST=$RADIUS_NODELIST[0];

my $radius = $ACCESS_REQUEST
	? Authen::Radius->new( Host=>$RADIUS_HOST, Secret=>$RADIUS_SECRET, NodeList=>\@RADIUS_NODELIST)
	: Authen::Simple::RADIUS->new( host=>$RADIUS_HOST, secret=>$RADIUS_SECRET);

die unless defined $radius;

if ($ACCESS_REQUEST) {
	Authen::Radius->load_dictionary($RADIUS_DICTIONARY);
}
    
use lib '/usr/local/lib/mavis';
use lib '/home/huber/DEVEL/PROJECTS/mavis/perl/'; # REMOVE #

use strict;
use Mavis;

$| = 1;

my ($in);

$/ = "\n=\n";

sub auth($$$$) {
	my ($r, $user, $pass, $nasip) = @_;
	if ($ACCESS_REQUEST) {
		$r->clear_attributes();
		$r->add_attributes(
			{ Name => 'User-Name', Value => $user, Type => 'string'},
			{ Name => 'Password', Value => $pass, Type => 'string'},
			{ Name => 'NAS-IP-Address', Value => $nasip, Type => 'ipaddr'}
		);
		$r->send_packet($ACCESS_REQUEST);
		my $rcv = $r->recv_packet();
		(defined($rcv) && $rcv == $ACCESS_ACCEPT) ? $r : undef;
	} else {
		$r->authenticate($user, $pass);
	}
}

while ($in = <>) {
	my ($a, @V, $result);

	@V = ();
	$result = MAVIS_DEFERRED;

	chomp $in;

	foreach $a (split (/\n/, $in)) {
		next unless $a =~ /^(\d+) (.*)$/;
		$V[$1] = $2;
	}
	if (!defined $V[AV_A_TYPE] || ($V[AV_A_TYPE] ne AV_V_TYPE_TACPLUS) ||
		!defined $V[AV_A_TACTYPE] || ($V[AV_A_TACTYPE] ne AV_V_TACTYPE_AUTH)){
		$result = MAVIS_DOWN;
	}
	elsif (!defined $V[AV_A_USER]){
		$V[AV_A_USER_RESPONSE] = "User not set.";
		$V[AV_A_RESULT] = AV_V_RESULT_ERROR;
		$result = MAVIS_FINAL;
	}
	elsif ($V[AV_A_USER] !~ /[-0-9a-z_]+/i){
		$V[AV_A_USER_RESPONSE] = "User name not valid.";
		$V[AV_A_RESULT] = AV_V_RESULT_ERROR;
		$result = MAVIS_FINAL;
	}
	elsif (!defined $V[AV_A_PASSWORD]){
		$V[AV_A_USER_RESPONSE] = "Password not set.";
		$V[AV_A_RESULT] = AV_V_RESULT_ERROR;
		$result = MAVIS_FINAL;
	}
	else {
		unless (($V[AV_A_TACTYPE] eq AV_V_TACTYPE_AUTH) &&
			auth($radius, $V[AV_A_USER], $V[AV_A_PASSWORD],
				exists ($V[AV_A_SERVERIP]) ? $V[AV_A_SERVERIP] : undef)) {
			$V[AV_A_USER_RESPONSE] = "Permission denied.";
			$V[AV_A_RESULT] = AV_V_RESULT_FAIL;
			$result = MAVIS_FINAL;
			goto bye;
		}

		if (defined $V[AV_A_PASSWORD] &&
						$V[AV_A_TACTYPE] eq AV_V_TACTYPE_AUTH){
				$V[AV_A_DBPASSWORD] = $V[AV_A_PASSWORD];
				$V[AV_A_PASSWORD_ONESHOT] = "1";
		}
		if (defined $RADIUS_GROUP_ATTR) {
			for my $a ($radius->get_attributes()) {
				if ($a->{'Name'} eq $RADIUS_GROUP_ATTR) {
					$V[AV_A_TACPROFILE] = '{ member = ' . $a->{'Value'} . ' }';
					last;
				}
			}
		}

		$V[AV_A_RESULT] = AV_V_RESULT_OK;
		$result = MAVIS_DOWN;
	}

bye:
	my ($out) = "";
	for (my $i = 0; $i <= $#V; $i++) {
			$out .= sprintf ("%d %s\n", $i, $V[$i]) if defined $V[$i];
	}
	$out .= sprintf ("=%d\n", $result);
	print $out;
}

# vim: ts=4
