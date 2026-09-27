import React from 'react';

interface ChartCardProps {
  title: string;
  children: React.ReactNode;
  className?: string;
}

export function ChartCard({ title, children, className = '' }: ChartCardProps) {
  return (
    <div className={`bg-[#1e2130] border border-[#2a2d3a] rounded-lg p-4 flex flex-col h-64 ${className}`}>
      <h3 className="text-sm font-semibold text-muted uppercase tracking-wider mb-3">{title}</h3>
      <div className="flex-1 w-full h-full min-h-0">
        {children}
      </div>
    </div>
  );
}
